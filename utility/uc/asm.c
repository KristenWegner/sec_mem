// asm.c - GAS-like assembler.


#include "all.h"


#ifdef CONFIG_TCC_ASM

ST_FUNC int asm_get_local_label_name(uc_state_t *s1, unsigned int n)
{
	char buf[64];
	uc_token_symbol_t *ts;

	snprintf(buf, sizeof(buf), "L..%u", n);
	ts = uc_pre_token_allocate(buf, strlen(buf));
	return ts->tok;
}

static int tcc_assemble_internal(uc_state_t *s1, int do_preprocess, int global);
static uc_symbol_t* asm_new_label(uc_state_t *s1, int label, int is_local);
static uc_symbol_t* asm_new_label1(uc_state_t *s1, int label, int is_local, int sh_num, int value);

static uc_symbol_t *asm_label_find(int v)
{
	uc_symbol_t *sym = sym_find(v);
	while (sym && sym->sym_scope)
		sym = sym->prev_tok;
	return sym;
}

static uc_symbol_t *asm_label_push(int v)
{
	/* We always add VT_EXTERN, for sym definition that's tentative
	   (for .set, removed for real defs), for mere references it's correct
	   as is.  */
	uc_symbol_t *sym = global_identifier_push(v, VT_ASM | VT_EXTERN | VT_STATIC, 0);
	sym->r = VT_CONST | VT_SYM;
	return sym;
}

/* Return a symbol we can use inside the assembler, having name NAME.
   Symbols from asm and C source share a namespace.  If we generate
   an asm symbol it's also a (file-global) C symbol, but it's
   either not accessible by name (like "L.123"), or its type information
   is such that it's not usable without a proper C declaration.

   Sometimes we need symbols accessible by name from asm, which
   are anonymous in C, in this case CSYM can be used to transfer
   all information from that symbol to the (possibly newly created)
   asm symbol.  */
ST_FUNC uc_symbol_t* get_asm_sym(int name, uc_symbol_t *csym)
{
	uc_symbol_t *sym = asm_label_find(name);
	if (!sym) {
		sym = asm_label_push(name);
		if (csym)
			sym->c = csym->c;
	}
	return sym;
}

static uc_symbol_t* asm_section_sym(uc_state_t *s1, uc_section_t *sec)
{
	char buf[100];
	int label = uc_pre_token_allocate(buf,
		snprintf(buf, sizeof buf, "L.%s", sec->name)
	)->tok;
	uc_symbol_t *sym = asm_label_find(label);
	return sym ? sym : asm_new_label1(s1, label, 1, sec->sh_num, 0);
}

/* We do not use the C expression parser to handle symbols. Maybe the
   C expression parser could be tweaked to do so. */

static void asm_expr_unary(uc_state_t *s1, uc_expression_value_t *pe)
{
	uc_symbol_t *sym;
	int op, label;
	uint64_t n;
	const char *p;

	switch (tok) {
	case TOK_PPNUM:
		p = tokc.str.data;
		n = strtoull(p, (char **)&p, 0);
		if (*p == 'b' || *p == 'f') {
			/* backward or forward label */
			label = asm_get_local_label_name(s1, n);
			sym = asm_label_find(label);
			if (*p == 'b') {
				/* backward : find the last corresponding defined label */
				if (sym && (!sym->c || elfsym(sym)->st_shndx == SHN_UNDEF))
					sym = sym->prev_tok;
				if (!sym)
					uc_error("local label '%d' not found backward", n);
			}
			else {
				/* forward */
				if (!sym || (sym->c && elfsym(sym)->st_shndx != SHN_UNDEF)) {
					/* if the last label is defined, then define a new one */
					sym = asm_label_push(label);
				}
			}
			pe->v = 0;
			pe->sym = sym;
			pe->pcrel = 0;
		}
		else if (*p == '\0') {
			pe->v = n;
			pe->sym = NULL;
			pe->pcrel = 0;
		}
		else {
			uc_error("invalid number syntax");
		}
		uc_pre_next_expansion();
		break;
	case '+':
		uc_pre_next_expansion();
		asm_expr_unary(s1, pe);
		break;
	case '-':
	case '~':
		op = tok;
		uc_pre_next_expansion();
		asm_expr_unary(s1, pe);
		if (pe->sym)
			uc_error("invalid operation with label");
		if (op == '-')
			pe->v = -pe->v;
		else
			pe->v = ~pe->v;
		break;
	case TOK_CCHAR:
	case TOK_LCHAR:
		pe->v = tokc.i;
		pe->sym = NULL;
		pe->pcrel = 0;
		uc_pre_next_expansion();
		break;
	case '(':
		uc_pre_next_expansion();
		asm_expr(s1, pe);
		skip(')');
		break;
	case '.':
		pe->v = ind;
		pe->sym = asm_section_sym(s1, cur_text_section);
		pe->pcrel = 0;
		uc_pre_next_expansion();
		break;
	default:
		if (tok >= TOK_IDENT) {
			ElfSym *esym;
			/* label case : if the label was not found, add one */
			sym = get_asm_sym(tok, NULL);
			esym = elfsym(sym);
			if (esym && esym->st_shndx == SHN_ABS) {
				/* if absolute symbol, no need to put a symbol value */
				pe->v = esym->st_value;
				pe->sym = NULL;
				pe->pcrel = 0;
			}
			else {
				pe->v = 0;
				pe->sym = sym;
				pe->pcrel = 0;
			}
			uc_pre_next_expansion();
		}
		else {
			uc_error("bad expression syntax [%s]", uc_get_token_string(tok, &tokc));
		}
		break;
	}
}

static void asm_expr_prod(uc_state_t *s1, uc_expression_value_t *pe)
{
	int op;
	uc_expression_value_t e2;

	asm_expr_unary(s1, pe);
	for (;;) {
		op = tok;
		if (op != '*' && op != '/' && op != '%' &&
			op != TOK_SHL && op != TOK_SAR)
			break;
		uc_pre_next_expansion();
		asm_expr_unary(s1, &e2);
		if (pe->sym || e2.sym)
			uc_error("invalid operation with label");
		switch (op) {
		case '*':
			pe->v *= e2.v;
			break;
		case '/':
			if (e2.v == 0) {
			div_error:
				uc_error("division by zero");
			}
			pe->v /= e2.v;
			break;
		case '%':
			if (e2.v == 0)
				goto div_error;
			pe->v %= e2.v;
			break;
		case TOK_SHL:
			pe->v <<= e2.v;
			break;
		default:
		case TOK_SAR:
			pe->v >>= e2.v;
			break;
		}
	}
}

static void asm_expr_logic(uc_state_t *s1, uc_expression_value_t *pe)
{
	int op;
	uc_expression_value_t e2;

	asm_expr_prod(s1, pe);
	for (;;) {
		op = tok;
		if (op != '&' && op != '|' && op != '^')
			break;
		uc_pre_next_expansion();
		asm_expr_prod(s1, &e2);
		if (pe->sym || e2.sym)
			uc_error("invalid operation with label");
		switch (op) {
		case '&':
			pe->v &= e2.v;
			break;
		case '|':
			pe->v |= e2.v;
			break;
		default:
		case '^':
			pe->v ^= e2.v;
			break;
		}
	}
}

static inline void asm_expr_sum(uc_state_t *s1, uc_expression_value_t *pe)
{
	int op;
	uc_expression_value_t e2;

	asm_expr_logic(s1, pe);
	for (;;) {
		op = tok;
		if (op != '+' && op != '-')
			break;
		uc_pre_next_expansion();
		asm_expr_logic(s1, &e2);
		if (op == '+') {
			if (pe->sym != NULL && e2.sym != NULL)
				goto cannot_relocate;
			pe->v += e2.v;
			if (pe->sym == NULL && e2.sym != NULL)
				pe->sym = e2.sym;
		}
		else {
			pe->v -= e2.v;
			/* NOTE: we are less powerful than gas in that case
			   because we store only one symbol in the expression */
			if (!e2.sym) {
				/* OK */
			}
			else if (pe->sym == e2.sym) {
				/* OK */
				pe->sym = NULL; /* same symbols can be subtracted to NULL */
			}
			else {
				ElfSym *esym1, *esym2;
				esym1 = elfsym(pe->sym);
				esym2 = elfsym(e2.sym);
				if (esym1 && esym1->st_shndx == esym2->st_shndx
					&& esym1->st_shndx != SHN_UNDEF) {
					/* we also accept defined symbols in the same section */
					pe->v += esym1->st_value - esym2->st_value;
					pe->sym = NULL;
				}
				else if (esym2->st_shndx == cur_text_section->sh_num) {
					/* When subtracting a defined symbol in current section
					   this actually makes the value PC-relative.  */
					pe->v -= esym2->st_value - ind - 4;
					pe->pcrel = 1;
					e2.sym = NULL;
				}
				else {
				cannot_relocate:
					uc_error("invalid operation with label");
				}
			}
		}
	}
}

static inline void asm_expr_cmp(uc_state_t *s1, uc_expression_value_t *pe)
{
	int op;
	uc_expression_value_t e2;

	asm_expr_sum(s1, pe);
	for (;;) {
		op = tok;
		if (op != TOK_EQ && op != TOK_NE
			&& (op > TOK_GT || op < TOK_ULE))
			break;
		uc_pre_next_expansion();
		asm_expr_sum(s1, &e2);
		if (pe->sym || e2.sym)
			uc_error("invalid operation with label");
		switch (op) {
		case TOK_EQ:
			pe->v = pe->v == e2.v;
			break;
		case TOK_NE:
			pe->v = pe->v != e2.v;
			break;
		case TOK_LT:
			pe->v = (int64_t)pe->v < (int64_t)e2.v;
			break;
		case TOK_GE:
			pe->v = (int64_t)pe->v >= (int64_t)e2.v;
			break;
		case TOK_LE:
			pe->v = (int64_t)pe->v <= (int64_t)e2.v;
			break;
		case TOK_GT:
			pe->v = (int64_t)pe->v > (int64_t)e2.v;
			break;
		default:
			break;
		}
		/* GAS compare results are -1/0 not 1/0.  */
		pe->v = -(int64_t)pe->v;
	}
}

ST_FUNC void asm_expr(uc_state_t *s1, uc_expression_value_t *pe)
{
	asm_expr_cmp(s1, pe);
}

ST_FUNC int asm_int_expr(uc_state_t *s1)
{
	uc_expression_value_t e;
	asm_expr(s1, &e);
	if (e.sym)
		uc_pre_expect("constant");
	return e.v;
}

static uc_symbol_t* asm_new_label1(uc_state_t *s1, int label, int is_local,
	int sh_num, int value)
{
	uc_symbol_t *sym;
	ElfSym *esym;

	sym = asm_label_find(label);
	if (sym) {
		esym = elfsym(sym);
		/* A VT_EXTERN symbol, even if it has a section is considered
		   overridable.  This is how we "define" .set targets.  Real
		   definitions won't have VT_EXTERN set.  */
		if (esym && esym->st_shndx != SHN_UNDEF) {
			/* the label is already defined */
			if (IS_ASM_SYM(sym)
				&& (is_local == 1 || (sym->type.t & VT_EXTERN)))
				goto new_label;
			if (!(sym->type.t & VT_EXTERN))
				uc_error("assembler label '%s' already defined",
					uc_get_token_string(label, NULL));
		}
	}
	else {
	new_label:
		sym = asm_label_push(label);
	}
	if (!sym->c)
		put_extern_sym2(sym, SHN_UNDEF, 0, 0, 0);
	esym = elfsym(sym);
	esym->st_shndx = sh_num;
	esym->st_value = value;
	if (is_local != 2)
		sym->type.t &= ~VT_EXTERN;
	return sym;
}

static uc_symbol_t* asm_new_label(uc_state_t *s1, int label, int is_local)
{
	return asm_new_label1(s1, label, is_local, cur_text_section->sh_num, ind);
}

/* Set the value of LABEL to that of some expression (possibly
   involving other symbols).  LABEL can be overwritten later still.  */
static uc_symbol_t* set_symbol(uc_state_t *s1, int label)
{
	long n;
	uc_expression_value_t e;
	uc_symbol_t *sym;
	ElfSym *esym;
	uc_pre_next_expansion();
	asm_expr(s1, &e);
	n = e.v;
	esym = elfsym(e.sym);
	if (esym)
		n += esym->st_value;
	sym = asm_new_label1(s1, label, 2, esym ? esym->st_shndx : SHN_ABS, n);
	elfsym(sym)->st_other |= ST_ASM_SET;
	return sym;
}

static void use_section1(uc_state_t *s1, uc_section_t *sec)
{
	cur_text_section->data_offset = ind;
	cur_text_section = sec;
	ind = cur_text_section->data_offset;
}

static void use_section(uc_state_t *s1, const char *name)
{
	uc_section_t *sec;
	sec = find_section(s1, name);
	use_section1(s1, sec);
}

static void push_section(uc_state_t *s1, const char *name)
{
	uc_section_t *sec = find_section(s1, name);
	sec->prev = cur_text_section;
	use_section1(s1, sec);
}

static void pop_section(uc_state_t *s1)
{
	uc_section_t *prev = cur_text_section->prev;
	if (!prev)
		uc_error(".popsection without .pushsection");
	cur_text_section->prev = NULL;
	use_section1(s1, prev);
}

static void asm_parse_directive(uc_state_t *s1, int global)
{
	int n, offset, v, size, tok1;
	uc_section_t *sec;
	uint8_t *ptr;

	/* assembler directive */
	sec = cur_text_section;
	switch (tok) {
	case TOK_ASMDIR_align:
	case TOK_ASMDIR_balign:
	case TOK_ASMDIR_p2align:
	case TOK_ASMDIR_skip:
	case TOK_ASMDIR_space:
		tok1 = tok;
		uc_pre_next_expansion();
		n = asm_int_expr(s1);
		if (tok1 == TOK_ASMDIR_p2align)
		{
			if (n < 0 || n > 30)
				uc_error("invalid p2align, must be between 0 and 30");
			n = 1 << n;
			tok1 = TOK_ASMDIR_align;
		}
		if (tok1 == TOK_ASMDIR_align || tok1 == TOK_ASMDIR_balign) {
			if (n < 0 || (n & (n - 1)) != 0)
				uc_error("alignment must be a positive power of two");
			offset = (ind + n - 1) & -n;
			size = offset - ind;
			/* the section must have a compatible alignment */
			if (sec->sh_addralign < n)
				sec->sh_addralign = n;
		}
		else {
			if (n < 0)
				n = 0;
			size = n;
		}
		v = 0;
		if (tok == ',') {
			uc_pre_next_expansion();
			v = asm_int_expr(s1);
		}
	zero_pad:
		if (sec->sh_type != SHT_NOBITS) {
			sec->data_offset = ind;
			ptr = section_ptr_add(sec, size);
			memset(ptr, v, size);
		}
		ind += size;
		break;
	case TOK_ASMDIR_quad:
#ifdef TCC_TARGET_X86_64
		size = 8;
		goto asm_data;
#else
		uc_pre_next_expansion();
		for (;;) {
			uint64_t vl;
			const char *p;

			p = tokc.str.data;
			if (tok != TOK_PPNUM) {
			error_constant:
				uc_error("64 bit constant");
			}
			vl = strtoll(p, (char **)&p, 0);
			if (*p != '\0')
				goto error_constant;
			uc_pre_next_expansion();
			if (sec->sh_type != SHT_NOBITS) {
				/* XXX: endianness */
				gen_le32(vl);
				gen_le32(vl >> 32);
			}
			else {
				ind += 8;
			}
			if (tok != ',')
				break;
			uc_pre_next_expansion();
		}
		break;
#endif
	case TOK_ASMDIR_byte:
		size = 1;
		goto asm_data;
	case TOK_ASMDIR_word:
	case TOK_ASMDIR_short:
		size = 2;
		goto asm_data;
	case TOK_ASMDIR_long:
	case TOK_ASMDIR_int:
		size = 4;
	asm_data:
		uc_pre_next_expansion();
		for (;;) {
			uc_expression_value_t e;
			asm_expr(s1, &e);
			if (sec->sh_type != SHT_NOBITS) {
				if (size == 4) {
					gen_expr32(&e);
#ifdef TCC_TARGET_X86_64
				}
				else if (size == 8) {
					gen_expr64(&e);
#endif
				}
				else {
					if (e.sym)
						uc_pre_expect("constant");
					if (size == 1)
						g(e.v);
					else
						gen_le16(e.v);
				}
			}
			else {
				ind += size;
			}
			if (tok != ',')
				break;
			uc_pre_next_expansion();
		}
		break;
	case TOK_ASMDIR_fill:
	{
		int repeat, size, val, i, j;
		uint8_t repeat_buf[8];
		uc_pre_next_expansion();
		repeat = asm_int_expr(s1);
		if (repeat < 0) {
			uc_error("repeat < 0; .fill ignored");
			break;
		}
		size = 1;
		val = 0;
		if (tok == ',') {
			uc_pre_next_expansion();
			size = asm_int_expr(s1);
			if (size < 0) {
				uc_error("size < 0; .fill ignored");
				break;
			}
			if (size > 8)
				size = 8;
			if (tok == ',') {
				uc_pre_next_expansion();
				val = asm_int_expr(s1);
			}
		}
		/* XXX: endianness */
		repeat_buf[0] = val;
		repeat_buf[1] = val >> 8;
		repeat_buf[2] = val >> 16;
		repeat_buf[3] = val >> 24;
		repeat_buf[4] = 0;
		repeat_buf[5] = 0;
		repeat_buf[6] = 0;
		repeat_buf[7] = 0;
		for (i = 0; i < repeat; ++i) {
			for (j = 0; j < size; j++) {
				g(repeat_buf[j]);
			}
		}
	}
	break;
	case TOK_ASMDIR_rept:
	{
		int repeat;
		uc_token_string_t *init_str;
		uc_pre_next_expansion();
		repeat = asm_int_expr(s1);
		init_str = uc_pre_allocate_token_string();
		while (uc_pre_next_expansion(), tok != TOK_ASMDIR_endr) {
			if (tok == CH_EOF)
				uc_error("we at end of file, .endr not found");
			uc_pre_token_string_add_token(init_str);
		}
		uc_pre_token_string_append(init_str, -1);
		uc_pre_token_string_append(init_str, 0);
		uc_pre_begin_macro(init_str, 1);
		while (repeat-- > 0) {
			tcc_assemble_internal(s1, (parse_flags & UC_OPTION_PARSE_PREPROCESS),
				global);
			macro_pointer = init_str->str;
		}
		uc_pre_end_macro();
		uc_pre_next_expansion();
		break;
	}
	case TOK_ASMDIR_org:
	{
		unsigned long n;
		uc_expression_value_t e;
		ElfSym *esym;
		uc_pre_next_expansion();
		asm_expr(s1, &e);
		n = e.v;
		esym = elfsym(e.sym);
		if (esym) {
			if (esym->st_shndx != cur_text_section->sh_num)
				uc_pre_expect("constant or same-section symbol");
			n += esym->st_value;
		}
		if (n < ind)
			uc_error("attempt to .org backwards");
		v = 0;
		size = n - ind;
		goto zero_pad;
	}
	break;
	case TOK_ASMDIR_set:
		uc_pre_next_expansion();
		tok1 = tok;
		uc_pre_next_expansion();
		/* Also accept '.set stuff', but don't do anything with this.
		   It's used in GAS to set various features like '.set mips16'.  */
		if (tok == ',')
			set_symbol(s1, tok1);
		break;
	case TOK_ASMDIR_globl:
	case TOK_ASMDIR_global:
	case TOK_ASMDIR_weak:
	case TOK_ASMDIR_hidden:
		tok1 = tok;
		do {
			uc_symbol_t *sym;
			uc_pre_next_expansion();
			sym = get_asm_sym(tok, NULL);
			if (tok1 != TOK_ASMDIR_hidden)
				sym->type.t &= ~VT_STATIC;
			if (tok1 == TOK_ASMDIR_weak)
				sym->a.weak = 1;
			else if (tok1 == TOK_ASMDIR_hidden)
				sym->a.visibility = STV_HIDDEN;
			update_storage(sym);
			uc_pre_next_expansion();
		} while (tok == ',');
		break;
	case TOK_ASMDIR_string:
	case TOK_ASMDIR_ascii:
	case TOK_ASMDIR_asciz:
	{
		const uint8_t *p;
		int i, size, t;

		t = tok;
		uc_pre_next_expansion();
		for (;;) {
			if (tok != TOK_STR)
				uc_pre_expect("string constant");
			p = tokc.str.data;
			size = tokc.str.size;
			if (t == TOK_ASMDIR_ascii && size > 0)
				size--;
			for (i = 0; i < size; ++i)
				g(p[i]);
			uc_pre_next_expansion();
			if (tok == ',') {
				uc_pre_next_expansion();
			}
			else if (tok != TOK_STR) {
				break;
			}
		}
	}
	break;
	case TOK_ASMDIR_text:
	case TOK_ASMDIR_data:
	case TOK_ASMDIR_bss:
	{
		char sname[64];
		tok1 = tok;
		n = 0;
		uc_pre_next_expansion();
		if (tok != ';' && tok != TOK_LINEFEED) {
			n = asm_int_expr(s1);
			uc_pre_next_expansion();
		}
		if (n)
			sprintf(sname, "%s%d", uc_get_token_string(tok1, NULL), n);
		else
			sprintf(sname, "%s", uc_get_token_string(tok1, NULL));
		use_section(s1, sname);
	}
	break;
	case TOK_ASMDIR_file:
	{
		char filename[512];

		filename[0] = '\0';
		uc_pre_next_expansion();

		if (tok == TOK_STR)
			pstrcat(filename, sizeof(filename), tokc.str.data);
		else
			pstrcat(filename, sizeof(filename), uc_get_token_string(tok, NULL));

		if (s1->warn_unsupported)
			uc_warn("ignoring .file %s", filename);

		uc_pre_next_expansion();
	}
	break;
	case TOK_ASMDIR_ident:
	{
		char ident[256];

		ident[0] = '\0';
		uc_pre_next_expansion();

		if (tok == TOK_STR)
			pstrcat(ident, sizeof(ident), tokc.str.data);
		else
			pstrcat(ident, sizeof(ident), uc_get_token_string(tok, NULL));

		if (s1->warn_unsupported)
			uc_warn("ignoring .ident %s", ident);

		uc_pre_next_expansion();
	}
	break;
	case TOK_ASMDIR_size:
	{
		uc_symbol_t *sym;

		uc_pre_next_expansion();
		sym = asm_label_find(tok);
		if (!sym) {
			uc_error("label not found: %s", uc_get_token_string(tok, NULL));
		}

		/* XXX .size name,label2-label1 */
		if (s1->warn_unsupported)
			uc_warn("ignoring .size %s,*", uc_get_token_string(tok, NULL));

		uc_pre_next_expansion();
		skip(',');
		while (tok != TOK_LINEFEED && tok != ';' && tok != CH_EOF) {
			uc_pre_next_expansion();
		}
	}
	break;
	case TOK_ASMDIR_type:
	{
		uc_symbol_t *sym;
		const char *newtype;

		uc_pre_next_expansion();
		sym = get_asm_sym(tok, NULL);
		uc_pre_next_expansion();
		skip(',');
		if (tok == TOK_STR) {
			newtype = tokc.str.data;
		}
		else {
			if (tok == '@' || tok == '%')
				uc_pre_next_expansion();
			newtype = uc_get_token_string(tok, NULL);
		}

		if (!strcmp(newtype, "function") || !strcmp(newtype, "STT_FUNC")) {
			sym->type.t = (sym->type.t & ~VT_BTYPE) | VT_FUNC;
		}
		else if (s1->warn_unsupported)
			uc_warn("change type of '%s' from 0x%x to '%s' ignored",
				uc_get_token_string(sym->v, NULL), sym->type.t, newtype);

		uc_pre_next_expansion();
	}
	break;
	case TOK_ASMDIR_pushsection:
	case TOK_ASMDIR_section:
	{
		char sname[256];
		int old_nb_section = s1->nb_sections;

		tok1 = tok;
		/* XXX: support more options */
		uc_pre_next_expansion();
		sname[0] = '\0';
		while (tok != ';' && tok != TOK_LINEFEED && tok != ',') {
			if (tok == TOK_STR)
				pstrcat(sname, sizeof(sname), tokc.str.data);
			else
				pstrcat(sname, sizeof(sname), uc_get_token_string(tok, NULL));
			uc_pre_next_expansion();
		}
		if (tok == ',') {
			/* skip section options */
			uc_pre_next_expansion();
			if (tok != TOK_STR)
				uc_pre_expect("string constant");
			uc_pre_next_expansion();
			if (tok == ',') {
				uc_pre_next_expansion();
				if (tok == '@' || tok == '%')
					uc_pre_next_expansion();
				uc_pre_next_expansion();
			}
		}
		last_text_section = cur_text_section;
		if (tok1 == TOK_ASMDIR_section)
			use_section(s1, sname);
		else
			push_section(s1, sname);
		/* If we just allocated a new section reset its alignment to
		   1.  new_section normally acts for GCC compatibility and
		   sets alignment to PTR_SIZE.  The assembler behaves different. */
		if (old_nb_section != s1->nb_sections)
			cur_text_section->sh_addralign = 1;
	}
	break;
	case TOK_ASMDIR_previous:
	{
		uc_section_t *sec;
		uc_pre_next_expansion();
		if (!last_text_section)
			uc_error("no previous section referenced");
		sec = cur_text_section;
		use_section1(s1, last_text_section);
		last_text_section = sec;
	}
	break;
	case TOK_ASMDIR_popsection:
		uc_pre_next_expansion();
		pop_section(s1);
		break;
#ifdef TCC_TARGET_I386
	case TOK_ASMDIR_code16:
	{
		uc_pre_next_expansion();
		s1->seg_size = 16;
	}
	break;
	case TOK_ASMDIR_code32:
	{
		uc_pre_next_expansion();
		s1->seg_size = 32;
	}
	break;
#endif
#ifdef TCC_TARGET_X86_64
	/* added for compatibility with GAS */
	case TOK_ASMDIR_code64:
		uc_pre_next_expansion();
		break;
#endif
	default:
		uc_error("unknown assembler directive '.%s'", uc_get_token_string(tok, NULL));
		break;
	}
}


/* assemble a file */
static int tcc_assemble_internal(uc_state_t *s1, int do_preprocess, int global)
{
	int opcode;
	int saved_parse_flags = parse_flags;

	parse_flags = UC_OPTION_PARSE_ASSEMBLER | UC_OPTION_PARSE_TOKEN_STRINGS;
	if (do_preprocess)
		parse_flags |= UC_OPTION_PARSE_PREPROCESS;
	for (;;) {
		uc_pre_next_expansion();
		if (tok == TOK_EOF)
			break;
		/* generate line number info */
		if (global && s1->do_debug)
			tcc_debug_line(s1);
		parse_flags |= UC_OPTION_PARSE_LINE_FEED; /* XXX: suppress that hack */
	redo:
		if (tok == '#') {
			/* horrible gas comment */
			while (tok != TOK_LINEFEED)
				uc_pre_next_expansion();
		}
		else if (tok >= TOK_ASMDIR_FIRST && tok <= TOK_ASMDIR_LAST) {
			asm_parse_directive(s1, global);
		}
		else if (tok == TOK_PPNUM) {
			const char *p;
			int n;
			p = tokc.str.data;
			n = strtoul(p, (char **)&p, 10);
			if (*p != '\0')
				uc_pre_expect("':'");
			/* new local label */
			asm_new_label(s1, asm_get_local_label_name(s1, n), 1);
			uc_pre_next_expansion();
			skip(':');
			goto redo;
		}
		else if (tok >= TOK_IDENT) {
			/* instruction or label */
			opcode = tok;
			uc_pre_next_expansion();
			if (tok == ':') {
				/* new label */
				asm_new_label(s1, opcode, 0);
				uc_pre_next_expansion();
				goto redo;
			}
			else if (tok == '=') {
				set_symbol(s1, opcode);
				goto redo;
			}
			else {
				asm_opcode(s1, opcode);
			}
		}
		/* end of line */
		if (tok != ';' && tok != TOK_LINEFEED)
			uc_pre_expect("end of line");
		parse_flags &= ~UC_OPTION_PARSE_LINE_FEED; /* XXX: suppress that hack */
	}

	parse_flags = saved_parse_flags;
	return 0;
}

/* Assemble the current file */
ST_FUNC int tcc_assemble(uc_state_t *s1, int do_preprocess)
{
	int ret;
	tcc_debug_start(s1);
	/* default section is text */
	cur_text_section = text_section;
	ind = cur_text_section->data_offset;
	want_no_code = 0;
	ret = tcc_assemble_internal(s1, do_preprocess, 1);
	cur_text_section->data_offset = ind;
	tcc_debug_end(s1);
	return ret;
}

/********************************************************************/
/* GCC inline asm support */

/* assemble the string 'str' in the current C compilation unit without
   C preprocessing. NOTE: str is modified by modifying the '\0' at the
   end */
static void tcc_assemble_inline(uc_state_t *s1, char *str, int len, int global)
{
	const int *saved_macro_ptr = macro_pointer;
	int dotid = uc_pre_set_id_number('.', IS_ID);

	uc_open_buffered_file(s1, ":asm:", len);
	memcpy(file->buffer, str, len);
	macro_pointer = NULL;
	tcc_assemble_internal(s1, 0, global);
	tcc_close();

	uc_pre_set_id_number('.', dotid);
	macro_pointer = saved_macro_ptr;
}

/* find a constraint by its number or id (gcc 3 extended
   syntax). return -1 if not found. Return in *pp in char after the
   constraint */
ST_FUNC int find_constraint(uc_assembler_operand_t *operands, int nb_operands,
	const char *name, const char **pp)
{
	int index;
	uc_token_symbol_t *ts;
	const char *p;

	if (uc_is_number(*name)) {
		index = 0;
		while (uc_is_number(*name)) {
			index = (index * 10) + (*name) - '0';
			name++;
		}
		if ((unsigned)index >= nb_operands)
			index = -1;
	}
	else if (*name == '[') {
		name++;
		p = strchr(name, ']');
		if (p) {
			ts = uc_pre_token_allocate(name, p - name);
			for (index = 0; index < nb_operands; index++) {
				if (operands[index].id == ts->tok)
					goto found;
			}
			index = -1;
		found:
			name = p + 1;
		}
		else {
			index = -1;
		}
	}
	else {
		index = -1;
	}
	if (pp)
		*pp = name;
	return index;
}

static void subst_asm_operands(uc_assembler_operand_t *operands, int nb_operands,
	uc_string_t *out_str, uc_string_t *in_str)
{
	int c, index, modifier;
	const char *str;
	uc_assembler_operand_t *op;
	uc_stack_value_t sv;

	uc_pre_string_create(out_str);
	str = in_str->data;
	for (;;) {
		c = *str++;
		if (c == '%') {
			if (*str == '%') {
				str++;
				goto uc_pre_string_add_char;
			}
			modifier = 0;
			if (*str == 'c' || *str == 'n' ||
				*str == 'b' || *str == 'w' || *str == 'h' || *str == 'k' ||
				*str == 'q' ||
				/* P in GCC would add "@PLT" to symbol refs in PIC mode,
				   and make literal operands not be decorated with '$'.  */
				*str == 'P')
				modifier = *str++;
			index = find_constraint(operands, nb_operands, str, &str);
			if (index < 0)
				uc_error("invalid operand reference after %%");
			op = &operands[index];
			sv = *op->vt;
			if (op->reg >= 0) {
				sv.r = op->reg;
				if ((op->vt->r & VT_VALMASK) == VT_LLOCAL && op->is_memory)
					sv.r |= VT_LVAL;
			}
			subst_asm_operand(out_str, &sv, modifier);
		}
		else {
		uc_pre_string_add_char:
			cstr_ccat(out_str, c);
			if (c == '\0')
				break;
		}
	}
}


static void parse_asm_operands(uc_assembler_operand_t *operands, int *nb_operands_ptr,
	int is_output)
{
	uc_assembler_operand_t *op;
	int nb_operands;

	if (tok != ':') {
		nb_operands = *nb_operands_ptr;
		for (;;) {
			uc_string_t astr;
			if (nb_operands >= MAX_ASM_OPERANDS)
				uc_error("too many asm operands");
			op = &operands[nb_operands++];
			op->id = 0;
			if (tok == '[') {
				uc_pre_next_expansion();
				if (tok < TOK_IDENT)
					uc_pre_expect("identifier");
				op->id = tok;
				uc_pre_next_expansion();
				skip(']');
			}
			parse_mult_str(&astr, "string constant");
			op->constraint = uc_malloc(astr.size);
			strcpy(op->constraint, astr.data);
			uc_pre_string_free(&astr);
			skip('(');
			gexpr();
			if (is_output) {
				if (!(vtop->type.t & VT_ARRAY))
					test_lvalue();
			}
			else {
				/* we want to avoid LLOCAL case, except when the 'm'
				   constraint is used. Note that it may come from
				   register storage, so we need to convert (reg)
				   case */
				if ((vtop->r & VT_LVAL) &&
					((vtop->r & VT_VALMASK) == VT_LLOCAL ||
					(vtop->r & VT_VALMASK) < VT_CONST) &&
					!strchr(op->constraint, 'm')) {
					gv(RC_INT);
				}
			}
			op->vt = vtop;
			skip(')');
			if (tok == ',') {
				uc_pre_next_expansion();
			}
			else {
				break;
			}
		}
		*nb_operands_ptr = nb_operands;
	}
}

/* parse the GCC asm() instruction */
ST_FUNC void asm_instr(void)
{
	uc_string_t astr, astr1;
	uc_assembler_operand_t operands[MAX_ASM_OPERANDS];
	int nb_outputs, nb_operands, i, must_subst, out_reg;
	uint8_t clobber_regs[NB_ASM_REGS];

	uc_pre_next_expansion();
	/* since we always generate the asm() instruction, we can ignore
	   volatile */
	if (tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2 || tok == TOK_VOLATILE3) {
		uc_pre_next_expansion();
	}
	parse_asm_str(&astr);
	nb_operands = 0;
	nb_outputs = 0;
	must_subst = 0;
	memset(clobber_regs, 0, sizeof(clobber_regs));
	if (tok == ':') {
		uc_pre_next_expansion();
		must_subst = 1;
		/* output args */
		parse_asm_operands(operands, &nb_operands, 1);
		nb_outputs = nb_operands;
		if (tok == ':') {
			uc_pre_next_expansion();
			if (tok != ')') {
				/* input args */
				parse_asm_operands(operands, &nb_operands, 0);
				if (tok == ':') {
					/* clobber list */
					/* XXX: handle registers */
					uc_pre_next_expansion();
					for (;;) {
						if (tok != TOK_STR)
							uc_pre_expect("string constant");
						asm_clobber(clobber_regs, tokc.str.data);
						uc_pre_next_expansion();
						if (tok == ',') {
							uc_pre_next_expansion();
						}
						else {
							break;
						}
					}
				}
			}
		}
	}
	skip(')');
	/* NOTE: we do not eat the ';' so that we can restore the current
	   token after the assembler parsing */
	if (tok != ';')
		uc_pre_expect("';'");

	/* save all values in the memory */
	save_regs(0);

	/* compute constraints */
	asm_compute_constraints(operands, nb_operands, nb_outputs,
		clobber_regs, &out_reg);

	/* substitute the operands in the asm string. No substitution is
	   done if no operands (GCC behaviour) */
#ifdef ASM_DEBUG
	printf("asm: \"%s\"\n", (char *)astr.data);
#endif
	if (must_subst) {
		subst_asm_operands(operands, nb_operands, &astr1, &astr);
		uc_pre_string_free(&astr);
	}
	else {
		astr1 = astr;
	}
#ifdef ASM_DEBUG
	printf("subst_asm: \"%s\"\n", (char *)astr1.data);
#endif

	/* generate loads */
	asm_gen_code(operands, nb_operands, nb_outputs, 0,
		clobber_regs, out_reg);

	/* assemble the string with tcc internal assembler */
	tcc_assemble_inline(this_state, astr1.data, astr1.size - 1, 0);

	/* restore the current C token */
	uc_pre_next_expansion();

	/* store the output values if needed */
	asm_gen_code(operands, nb_operands, nb_outputs, 1,
		clobber_regs, out_reg);

	/* free everything */
	for (i = 0; i < nb_operands; ++i) {
		uc_assembler_operand_t *op;
		op = &operands[i];
		uc_free(op->constraint);
		vpop();
	}
	uc_pre_string_free(&astr1);
}

ST_FUNC void asm_global_instr(void)
{
	uc_string_t astr;
	int saved_nocode_wanted = want_no_code;

	/* Global asm blocks are always emitted.  */
	want_no_code = 0;
	uc_pre_next_expansion();
	parse_asm_str(&astr);
	skip(')');
	/* NOTE: we do not eat the ';' so that we can restore the current
	   token after the assembler parsing */
	if (tok != ';')
		uc_pre_expect("';'");

#ifdef ASM_DEBUG
	printf("asm_global: \"%s\"\n", (char *)astr.data);
#endif

	cur_text_section = text_section;
	ind = cur_text_section->data_offset;

	/* assemble the string with tcc internal assembler */
	tcc_assemble_inline(this_state, astr.data, astr.size - 1, 1);

	cur_text_section->data_offset = ind;

	/* restore the current C token */
	uc_pre_next_expansion();

	uc_pre_string_free(&astr);
	want_no_code = saved_nocode_wanted;
}

#endif // CONFIG_TCC_ASM


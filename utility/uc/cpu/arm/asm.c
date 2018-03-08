// asm.c - ARMv4 dummy assembler.


#include "../../all.h"


#define CONFIG_TCC_ASM 1
#define NB_ASM_REGS 16


ST_FUNC void g(int c);
ST_FUNC void gen_le16(int c);
ST_FUNC void gen_le32(int c);


static void asm_error(void)
{
	tcc_error("ARM assembler is not implemented.");
}


ST_FUNC void g(int c)
{
	int ind1;

	if (nocode_wanted)
		return;

	ind1 = ind + 1;

	if (ind1 > cur_text_section->data_allocated)
		section_realloc(cur_text_section, ind1);

	cur_text_section->data[ind] = c;
	ind = ind1;
}


ST_FUNC void gen_le16(int i)
{
	g(i);
	g(i >> 8);
}


ST_FUNC void gen_le32(int i)
{
	gen_le16(i);
	gen_le16(i >> 16);
}


ST_FUNC void gen_expr32(uc_expression_value_t *pe)
{
	gen_le32(pe->v);
}


ST_FUNC void asm_opcode(uc_state_t *s1, int opcode)
{
	asm_error();
}


ST_FUNC void subst_asm_operand(uc_string_t *add_str, uc_stack_value_t *sv, int modifier)
{
	asm_error();
}


/* generate prolog and epilog code for asm statement */
ST_FUNC void asm_gen_code(uc_assembler_operand_t *operands, int nb_operands,
	int nb_outputs, int is_output,
	uint8_t *clobber_regs,
	int out_reg)
{
}


ST_FUNC void asm_compute_constraints(uc_assembler_operand_t *operands,
	int nb_operands, int nb_outputs,
	const uint8_t *clobber_regs,
	int *pout_reg)
{
}


ST_FUNC void asm_clobber(uint8_t *clobber_regs, const char *str)
{
	asm_error();
}


ST_FUNC int asm_parse_regvar(int t)
{
	asm_error();
	return -1;
}



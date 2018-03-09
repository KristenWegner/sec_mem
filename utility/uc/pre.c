// pre.c - Preprocessor.


#include "all.h"
#include "api.h"


/* global variables */


#undef ST_DATA
#define ST_DATA


int token_flags;
int parse_flags;
struct uc_buffered_file_s* file;
int character, token;
uc_const_value_t token_constant;
const int* macro_pointer;
uc_string_t current_token; /* current parsed string, if any */

/* display benchmark infos */
ST_DATA int total_lines;
ST_DATA int total_bytes;
ST_DATA int total_identifiers;
ST_DATA uc_token_symbol_t **identifier_table;

static uc_token_symbol_t *identifier_hash[UC_LIMIT_HASH_SIZE];
static char token_buffer[UC_LIMIT_STRING_MAXIMUM_SIZE + 1];
static uc_string_t string_buffer;
static uc_string_t macro_resolution_buffer;
static uc_token_string_t token_string_buffer;
static unsigned char is_id_number_table[256 - CH_EOF];
static int preprocessor_debug_token, preprocessor_debug_symbol;
static int preprocessor_once;
static int preprocessor_expression;
static int preprocessor_counter;
static struct uc_allocator_s *token_symbol_allocator;
static struct uc_allocator_s *token_string_allocator;
static struct uc_allocator_s *string_allocator;
static uc_token_string_t *macro_stack;



const char uc_pre_key_words_table[] =
#define DEF(I, S) S "\0"
#include "tok.h"
#undef DEF
;


// Warning: The content of this string encodes token numbers.
const uint8_t uc_pre_two_character_token_table[] =
{
	'<','=', TOK_LE,
	'>','=', TOK_GE,
	'!','=', TOK_NE,
	'&','&', TOK_LAND,
	'|','|', TOK_LOR,
	'+','+', TOK_INC,
	'-','-', TOK_DEC,
	'=','=', TOK_EQ,
	'<','<', TOK_SHL,
	'>','>', TOK_SAR,
	'+','=', TOK_A_ADD,
	'-','=', TOK_A_SUB,
	'*','=', TOK_A_MUL,
	'/','=', TOK_A_DIV,
	'%','=', TOK_A_MOD,
	'&','=', TOK_A_AND,
	'^','=', TOK_A_XOR,
	'|','=', TOK_A_OR,
	'-','>', TOK_ARROW,
	'.','.', TOK_TWODOTS,
	'#','#', TOK_TWOSHARPS,
	0
};


static void uc_pre_print_token(const char*, const int32_t*);
static void uc_pre_next_non_macro_space(void);


void uc_pre_skip(int32_t c)
{
	if (token != c)
		uc_error("Expected '%c' but found \"%s\".", c, uc_get_token_string(token, &token_constant));

	uc_pre_next_expansion();
}


void uc_pre_expected(const char* message)
{
	uc_error("%s expected", message);
}


// Custom allocator for tiny objects.


#if !defined(UC_OPTION_DEBUG_MEMORY)

#define UC_ALLOCATOR_FREE(A, P)				uc_pre_allocator_free(A, P)
#define UC_ALLOCATOR_REALLOC(A, P, S)		uc_pre_allocator_realloc(&A, P, S)
#define UC_ALLOCATOR_DEBUG_PARAMS

#else

#define UC_OPTION_DEBUG_ALLOCATOR			1
#define UC_ALLOCATOR_FREE(A, P)				uc_pre_allocator_free(A, P, __FILE__, __LINE__)
#define UC_ALLOCATOR_REALLOC(A, P, S)		uc_pre_allocator_realloc(&A, P, S, __FILE__, __LINE__)
#define UC_ALLOCATOR_DEBUG_PARAMS			, const char* file, int32_t line
#define UC_ALLOCATOR_DEBUG_FILE_LENGTH		40

#endif


#define UC_ALLOCATOR_TOKEN_SYMBOL_SIZE		(768 * 1024) // Allocator for tiny uc_token_symbol_t in identifier_table.
#define UC_ALLOCATOR_TOKEN_STRING_SIZE		(768 * 1024) // Allocator for tiny uc_token_string_t instances.
#define UC_ALLOCATOR_STRING_SIZE			(256 * 1024) // Allocator for tiny uc_string_t instances.


#define UC_ALLOCATOR_TOKEN_SYMBOL_LIMIT		256 // Prefer unique limits to distinguish allocators debug messages.
#define UC_ALLOCATOR_TOKEN_STRING_LIMIT		128 // 32 * sizeof(int).
#define UC_ALLOCATOR_STRING_LIMIT			1024


typedef struct uc_allocator_s
{
	uint32_t limit;
	uint32_t size;

	uint8_t* buffer;
	uint8_t* p;

	uint32_t nb_allocs;

	struct uc_allocator_s* next;
	struct uc_allocator_s* top;

#ifdef UC_OPTION_ALLOCATOR_INFO
	uint32_t nb_peak;
	uint32_t nb_total;
	uint32_t nb_missed;
	uint8_t* peak_p;
#endif
}
uc_allocator_t;


typedef struct uc_allocator_header_s
{
	uint32_t size;

#ifdef UC_OPTION_DEBUG_ALLOCATOR
	int32_t line_number; // Negative line number used for double free check.
	char file_name[UC_ALLOCATOR_DEBUG_FILE_LENGTH + 1];
#endif
}
uc_allocator_header_t;


static uc_allocator_t* uc_pre_allocator_create(uc_allocator_t** allocator, uint32_t limit, uint32_t size)
{
	uc_allocator_t* result = uc_malloc_z(sizeof(uc_allocator_t));

	result->p = result->buffer = uc_malloc(size);
	result->limit = limit;
	result->size = size;

	if (allocator) *allocator = result;

	return result;
}

static void uc_pre_allocator_destroy(uc_allocator_t* allocator)
{
	uc_allocator_t* next;

LOC_TAIL_CALL:

	if (!allocator) return;

#ifdef UC_OPTION_ALLOCATOR_INFO
	fprintf(stderr, "uc_pre_allocator: limit = %d, size = %g MB, peak = %d, total = %d, missed = %d, usage = %f%%\n",
		allocator->limit, 
		allocator->size / 1024.0 / 1024.0, 
		allocator->nb_peak, 
		allocator->nb_total, 
		allocator->nb_missed,
		(allocator->peak_p - allocator->buffer) * 100.0 / allocator->size);
#endif

#ifdef UC_OPTION_DEBUG_ALLOCATOR
	if (allocator->nb_allocs > 0) 
	{
		fprintf(stderr, "uc_pre_allocator: memory leak (%d) chunks, limit = %d\n", 
			allocator->nb_allocs, allocator->limit);

		uint8_t* p = allocator->buffer;

		while (p < allocator->p) 
		{
			uc_allocator_header_t* h = (uc_allocator_header_t*)p;

			if (h->line_number > 0) 
				fprintf(stderr, "uc_pre_allocator: %s (%d): chunk of (%d) bytes leaked\n", 
					h->file_name, h->line_number, h->size);

			p += h->size + sizeof(uc_allocator_header_t);
		}

#if UC_OPTION_DEBUG_MEMORY == 2
		exit(2);
#endif
	}
#endif

	next = allocator->next;

	uc_free(allocator->buffer);
	uc_free(allocator);

	allocator = next;

	goto LOC_TAIL_CALL;
}


static void uc_pre_allocator_free(uc_allocator_t* allocator, void* p UC_ALLOCATOR_DEBUG_PARAMS)
{
	if (!p) return;

LOC_TAIL_CALL:

	if (allocator->buffer <= (uint8_t *)p && (uint8_t *)p < allocator->buffer + allocator->size)
	{

#ifdef UC_OPTION_DEBUG_ALLOCATOR
		uc_allocator_header_t* h = (((uc_allocator_header_t*)p) - 1);

		if (header->line_number < 0) 
		{
			fprintf(stderr, "uc_pre_allocator: %s (%d): Double frees chunk from:\n", file, line);
			fprintf(stderr, "uc_pre_allocator: %s (%d): %d bytes\n", h->file_name, (int)-h->line_number, (int)h->size);
		}
		else h->line_number = -h->line_number;
#endif

		allocator->nb_allocs--;

		if (!allocator->nb_allocs)
			allocator->p = allocator->buffer;
	}
	else if (allocator->next)
	{
		allocator = allocator->next;
		goto LOC_TAIL_CALL;
	}
	else uc_free(p);
}


static void* uc_pre_allocator_realloc(uc_allocator_t** allocator, void* p, uint32_t size UC_ALLOCATOR_DEBUG_PARAMS)
{
	uc_allocator_header_t* header;
	void* result;
	bool is_own;
	uint32_t adj_size = (size + 3) & -4;
	uc_allocator_t *al = *allocator;

LOC_TAIL_CALL:

	is_own = (al->buffer <= (uint8_t*)p && (uint8_t *)p < al->buffer + al->size);

	if ((!p || is_own) && size <= al->limit) 
	{
		if (al->p + adj_size + sizeof(uc_allocator_header_t) < al->buffer + al->size) 
		{
			header = (uc_allocator_header_t*)al->p;
			header->size = adj_size;

#ifdef UC_OPTION_DEBUG_ALLOCATOR
			{ 
				int32_t ofs = strlen(file) - UC_ALLOCATOR_DEBUG_FILE_LENGTH;
				strncpy(header->file_name, file + (ofs > 0 ? ofs : 0), UC_ALLOCATOR_DEBUG_FILE_LENGTH);
				header->file_name[UC_ALLOCATOR_DEBUG_FILE_LENGTH] = 0;
				header->line_number = line;
			}
#endif

			result = al->p + sizeof(uc_allocator_header_t);
			al->p += adj_size + sizeof(uc_allocator_header_t);

			if (is_own) 
			{
				header = (((uc_allocator_header_t*)p) - 1);
				memcpy(result, p, header->size);

#ifdef UC_OPTION_DEBUG_ALLOCATOR
				header->line_number = -header->line_number;
#endif
			}
			else al->nb_allocs++;

#ifdef UC_OPTION_ALLOCATOR_INFO
			if (al->nb_peak < al->nb_allocs)
				al->nb_peak = al->nb_allocs;

			if (al->peak_p < al->p)
				al->peak_p = al->p;

			al->nb_total++;
#endif

			return result;
		}
		else if (is_own) 
		{
			al->nb_allocs--;
			result = UC_ALLOCATOR_REALLOC(*allocator, 0, size);
			header = (((uc_allocator_header_t*)p) - 1);
			memcpy(result, p, header->size);

#ifdef UC_OPTION_DEBUG_ALLOCATOR
			header->line_number = -header->line_number;
#endif

			return result;
		}

		if (al->next) al = al->next;
		else 
		{
			uc_allocator_t* bottom = al, *next = al->top ? al->top : al;

			al = uc_pre_allocator_create(allocator, next->limit, next->size * 2);

			al->next = next;
			bottom->top = al;
		}

		goto LOC_TAIL_CALL;
	}

	if (is_own) 
	{
		al->nb_allocs--;

		result = uc_malloc(size);

		header = (((uc_allocator_header_t*)p) - 1);
		memcpy(result, p, header->size);

#ifdef UC_OPTION_DEBUG_ALLOCATOR
		header->line_number = -header->line_number;
#endif

	}
	else if (al->next)
	{
		al = al->next;

		goto LOC_TAIL_CALL;
	}
	else result = uc_realloc(p, size);

#ifdef UC_OPTION_ALLOCATOR_INFO
	al->nb_missed++;
#endif

	return result;
}


// String handling.


static void uc_pre_string_realloc(uc_string_t* s, int32_t n)
{
	int32_t size = s->size_allocated;

	if (size < 8) size = 8; // No need to allocate a small first string.

	while (size < n)
		size = size * 2;

	s->data = UC_ALLOCATOR_REALLOC(string_allocator, s->data, size);
	s->size_allocated = size;
}


// Append a character.
void uc_pre_string_char_cat(uc_string_t* s, int32_t character)
{
	int32_t size = s->size + 1;

	if (size > s->size_allocated)
		uc_pre_string_realloc(s, size);

	((uint8_t*)s->data)[size - 1] = character;

	s->size = size;
}


void uc_pre_string_cat(uc_string_t* s, const char* v, int32_t n)
{
	if (n < 1)
		n = strlen(v) + 1 + n;

	int32_t size = s->size + n;

	if (size > s->size_allocated)
		uc_pre_string_realloc(s, size);

	memmove(((uint8_t*)s->data) + s->size, v, n);

	s->size = size;
}


// Append a wide character.
void uc_pre_string_wchar_cat(uc_string_t* s, int32_t character)
{
	int32_t n = s->size + sizeof(nwchar_t);

	if (n > s->size_allocated)
		uc_pre_string_realloc(s, n);

	*(nwchar_t*)(((uint8_t*)s->data) + n - sizeof(nwchar_t)) = character;

	s->size = n;
}


void uc_pre_string_create(uc_string_t* s)
{
	if (s) memset(s, 0, sizeof(uc_string_t));
}


// Free string and reset it to null.
void uc_pre_string_free(uc_string_t* s)
{
	if (!s) return;

	UC_ALLOCATOR_FREE(string_allocator, s->data);
	uc_pre_string_create(s);
}


// Reset string to empty.
void uc_pre_string_reset(uc_string_t* s)
{
	if (s) s->size = 0;
}


static void uc_pre_string_add_char(uc_string_t* s, int32_t c)
{
	if (c == '\'' || c == '\"' || c == '\\')
		uc_pre_string_char_cat(s, '\\');
	
	if (c >= 32 && c <= 126)
		uc_pre_string_char_cat(s, c);
	else 
	{
		uc_pre_string_char_cat(s, '\\');

		if (c == '\n')
			uc_pre_string_char_cat(s, 'n');
		else 
		{
			uc_pre_string_char_cat(s, '0' + ((c >> 6) & 7));
			uc_pre_string_char_cat(s, '0' + ((c >> 3) & 7));
			uc_pre_string_char_cat(s, '0' + (c & 7));
		}
	}
}


// Allocate a new token.
static uc_token_symbol_t* uc_pre_token_allocate_new(uc_token_symbol_t** pts, const char* str, int32_t len)
{
	uc_token_symbol_t *ts, **pt;

	if (total_identifiers >= SYM_FIRST_ANOM)
		uc_error("Preprocessor: Symbol memory is full.");

	// Expand token table if needed.

	int32_t i = total_identifiers - TOK_IDENT;

	if ((i % TOK_ALLOC_INCR) == 0) 
	{
		pt = uc_realloc(identifier_table, (i + TOK_ALLOC_INCR) * sizeof(uc_token_symbol_t *));
		identifier_table = pt;
	}

	ts = UC_ALLOCATOR_REALLOC(token_symbol_allocator, 0, sizeof(uc_token_symbol_t) + len);

	identifier_table[i] = ts;

	ts->tok = total_identifiers++;
	ts->sym_define = NULL;
	ts->sym_label = NULL;
	ts->sym_struct = NULL;
	ts->sym_identifier = NULL;
	ts->len = len;
	ts->hash_next = NULL;

	memcpy(ts->str, str, len);

	ts->str[len] = '\0';
	*pts = ts;

	return ts;
}


#define UC_PRE_TOKEN_HASH_INITIALIZER		1
#define UC_PRE_TOKEN_HASH_FUNCTION(H, C)	((H) + ((H) << 5) + ((H) >> 27) + (C))


// Find a token and add it if not found.
uc_token_symbol_t* uc_pre_token_allocate(const char* s, int32_t n)
{
	uc_token_symbol_t *ts, **pt;
	int32_t i;

	uint32_t h = UC_PRE_TOKEN_HASH_INITIALIZER;

	for (i = 0; i < n; ++i)
		h = UC_PRE_TOKEN_HASH_FUNCTION(h, ((uint8_t*)s)[i]);

	h &= (UC_LIMIT_HASH_SIZE - 1);

	pt = &identifier_hash[h];

	for (;;) 
	{
		ts = *pt;

		if (!ts) break;

		if (ts->len == n && !memcmp(ts->str, s, n))
			return ts;

		pt = &(ts->hash_next);
	}

	return uc_pre_token_allocate_new(pt, s, n);
}


const char* uc_get_token_string(int32_t v, uc_const_value_t* c)
{
	int32_t i, n;

	uc_pre_string_reset(&string_buffer);
	char* p = string_buffer.data;

	switch (v)
	{
	case TOK_CINT:
	case TOK_CUINT:
	case TOK_CLONG:
	case TOK_CULONG:
	case TOK_CLLONG:
	case TOK_CULLONG:
#ifdef _WIN32
		sprintf(p, "%u", (unsigned)c->i);
#else
		sprintf(p, "%llu", (unsigned long long)c->i);
#endif
		break;

	case TOK_LCHAR:
		uc_pre_string_char_cat(&string_buffer, 'L');

	case TOK_CCHAR:
		uc_pre_string_char_cat(&string_buffer, '\'');
		uc_pre_string_add_char(&string_buffer, c->i);
		uc_pre_string_char_cat(&string_buffer, '\'');
		uc_pre_string_char_cat(&string_buffer, '\0');
		break;

	case TOK_PPNUM:
	case TOK_PPSTR:
		return (char*)c->str.data;

	case TOK_LSTR:
		uc_pre_string_char_cat(&string_buffer, 'L');

	case TOK_STR:

		uc_pre_string_char_cat(&string_buffer, '\"');

		if (v == TOK_STR) 
		{
			n = c->str.size - 1;

			for (i = 0; i < n; ++i)
				uc_pre_string_add_char(&string_buffer, ((uint8_t*)c->str.data)[i]);
		}
		else 
		{
			n = (c->str.size / sizeof(nwchar_t)) - 1;

			for (i = 0; i < n; ++i)
				uc_pre_string_add_char(&string_buffer, ((nwchar_t*)c->str.data)[i]);
		}

		uc_pre_string_char_cat(&string_buffer, '\"');
		uc_pre_string_char_cat(&string_buffer, '\0');

		break;

	case TOK_CFLOAT:
		uc_pre_string_cat(&string_buffer, "<float>", 0);
		break;

	case TOK_CDOUBLE:
		uc_pre_string_cat(&string_buffer, "<double>", 0);
		break;

	case TOK_CLDOUBLE:
		uc_pre_string_cat(&string_buffer, "<long double>", 0);
		break;

	case TOK_LINENUM:
		uc_pre_string_cat(&string_buffer, "<linenumber>", 0);
		break;

		// The above tokens have value, the ones below don't.

	case TOK_LT:
		v = '<';
		goto LOC_ADD_V;

	case TOK_GT:
		v = '>';
		goto LOC_ADD_V;

	case TOK_DOTS:
		return strcpy(p, "...");

	case TOK_A_SHL:
		return strcpy(p, "<<=");

	case TOK_A_SAR:
		return strcpy(p, ">>=");

	case TOK_EOF:
		return strcpy(p, "<eof>");

	default:

		if (v < TOK_IDENT) 
		{
			// Search in two byte table.
			const uint8_t* q = uc_pre_two_character_token_table;

			while (*q) 
			{
				if (q[2] == v) 
				{
					*p++ = q[0];
					*p++ = q[1];
					*p = '\0';

					return string_buffer.data;
				}

				q += 3;
			}

			if (v >= 127) 
			{
				sprintf(string_buffer.data, "<%02x>", v);

				return string_buffer.data;
			}

		LOC_ADD_V:

			*p++ = v;
			*p = '\0';
		}
		else if (v < total_identifiers) 
			return identifier_table[v - TOK_IDENT]->str;
		else if (v >= SYM_FIRST_ANOM) 
			sprintf(p, "L.%u", v - SYM_FIRST_ANOM); // Special name for anonymous symbol.
		else return NULL; // Should never happen.

		break;
	}

	return string_buffer.data;
}


// Return the current character, handling end of block if necessary (but not stray).
int32_t uc_pre_handle_end_of_buffer(void)
{
	uc_buffered_file_t* bf = file;
	int32_t len;

	// Only attempts to read if actually at end of buffer.

	if (bf->buf_ptr >= bf->buf_end) 
	{
		if (bf->fd >= 0) 
		{
#if defined(UC_OPTION_DEBUG_PARSE)
			len = 1;
#else
			len = IO_BUF_SIZE;
#endif

			len = read(bf->fd, bf->buffer, len);

			if (len < 0)
				len = 0;
		}
		else  len = 0;

		total_bytes += len;
		bf->buf_ptr = bf->buffer;
		bf->buf_end = bf->buffer + len;
		*(bf->buf_end) = CH_EOB;
	}

	if (bf->buf_ptr < bf->buf_end) 
		return bf->buf_ptr[0];
	else 
	{
		bf->buf_ptr = bf->buf_end;

		return CH_EOF;
	}
}


// Read uc_pre_next_expansion character from current input file and handles end of input buffer.
inline static void uc_pre_in(void)
{
	character = *(++(file->buf_ptr));

	// End of buffer / file handling.

	if (character == CH_EOB)
		character = uc_pre_handle_end_of_buffer();
}


// Handle '\[\r]\n'.
static bool uc_pre_handle_stray_no_error(void)
{
	while (character == '\\') 
	{
		uc_pre_in();

		if (character == '\n') 
		{
			file->line_num++;

			uc_pre_in();
		}
		else if (character == '\r') 
		{
			uc_pre_in();

			if (character != '\n')
				goto LOC_FAIL;

			file->line_num++;

			uc_pre_in();
		}
		else 
		{
		LOC_FAIL:

			return true;
		}
	}

	return false;
}


static void uc_pre_handle_stray(void)
{
	if (uc_pre_handle_stray_no_error())
		uc_error("Stray '\\' discovered in source.");
}


// Skip the stray and handle the \\n case. Output an error if incorrect char 
// after the stray.
static int32_t uc_pre_handle_stray_1(uint8_t* p)
{
	int32_t c;

	file->buf_ptr = p;

	if (p >= file->buf_end) 
	{
		c = uc_pre_handle_end_of_buffer();

		if (c != '\\')
			return c;

		p = file->buf_ptr;
	}

	character = *p;

	if (uc_pre_handle_stray_no_error()) 
	{
		if (!(parse_flags & UC_OPTION_PARSE_ACCEPT_STRAYS))
			uc_error("stray '\\' in program");

		*--file->buf_ptr = '\\';
	}

	p = file->buf_ptr;
	c = *p;

	return c;
}


// Handle just the EOB case, but not stray.
#define PRE_PEEK_C_EOB(C, P) { P++; C = *P; if (C == '\\') { file->buf_ptr = P; C = uc_pre_handle_end_of_buffer(); P = file->buf_ptr; } }


// Handle the complicated stray case.
#define PRE_PEEK_C(C, P) { P++; C = *P; if (C == '\\') { C = uc_pre_handle_stray_1(P); P = file->buf_ptr; } }


// Input with '\[\r]\n' handling. Note that this function cannot handle other characters 
// after '\', so you cannot call it inside strings or comments.
void uc_pre_m_in(void)
{
	uc_pre_in();

	if (character == '\\')
		uc_pre_handle_stray();
}


// Single line comments.
static uint8_t* uc_pre_parse_single_line_comment(uint8_t* p)
{
	int32_t c;

	p++;

	for (;;) 
	{
		c = *p;

	LOC_REDO:

		if (c == '\n' || c == CH_EOF) break;
		else if (c == '\\') 
		{
			file->buf_ptr = p;
			c = uc_pre_handle_end_of_buffer();
			p = file->buf_ptr;

			if (c == '\\') 
			{
				PRE_PEEK_C_EOB(c, p);

				if (c == '\n') 
				{
					file->line_num++;
					PRE_PEEK_C_EOB(c, p);
				}
				else if (c == '\r') 
				{
					PRE_PEEK_C_EOB(c, p);

					if (c == '\n') 
					{
						file->line_num++;
						PRE_PEEK_C_EOB(c, p);
					}
				}
			}
			else goto LOC_REDO;
		}
		else p++;
	}

	return p;
}


// Multi-line comments.
uint8_t* uc_pre_parse_multiline_comment(uint8_t* p)
{
	int c;

	p++;

	for (;;) 
	{
		// Fast skip loop.
		for (;;) 
		{
			c = *p;

			if (c == '\n' || c == '*' || c == '\\')
				break;

			p++;
			c = *p;

			if (c == '\n' || c == '*' || c == '\\')
				break;

			p++;
		}

		// Now handle all the cases.
		if (c == '\n') 
		{
			file->line_num++;
			p++;
		}
		else if (c == '*') 
		{
			p++;

			for (;;) 
			{
				c = *p;

				if (c == '*') p++;
				else if (c == '/') goto LOC_END_OF_COMMENT;
				else if (c == '\\') 
				{
					file->buf_ptr = p;

					c = uc_pre_handle_end_of_buffer();
					p = file->buf_ptr;

					if (c == CH_EOF)
						uc_error("Unexpected end of file in multiline comment.");

					if (c == '\\') 
					{
						// Skip '\[\r]\n', otherwise just skip the stray.

						while (c == '\\') 
						{
							PRE_PEEK_C_EOB(c, p);

							if (c == '\n') 
							{
								file->line_num++;
								PRE_PEEK_C_EOB(c, p);
							}
							else if (c == '\r') 
							{
								PRE_PEEK_C_EOB(c, p);

								if (c == '\n') 
								{
									file->line_num++;
									PRE_PEEK_C_EOB(c, p);
								}
							}
							else goto LOC_AFTER_ASTERISK;
						}
					}
				}
				else break;
			}

		LOC_AFTER_ASTERISK:;
		}
		else 
		{
			// Stray, EOB or EOF.

			file->buf_ptr = p;
			c = uc_pre_handle_end_of_buffer();
			p = file->buf_ptr;

			if (c == CH_EOF) 
				uc_error("Unexpected end of file in multiline comment.");
			else if (c == '\\') 
				p++;
		}
	}

LOC_END_OF_COMMENT:

	p++;

	return p;
}


int32_t uc_pre_set_id_number(int32_t c, int32_t v)
{
	int prev = is_id_number_table[c - CH_EOF];
	is_id_number_table[c - CH_EOF] = v;
	return prev;
}


inline static void uc_pre_skip_space(void)
{
	while (is_id_number_table[character - CH_EOF] & IS_SPC)
		uc_pre_m_in();
}


inline static int uc_pre_check_space(int32_t t, int32_t* s)
{
	if (t < 256 && (is_id_number_table[t - CH_EOF] & IS_SPC))
	{
		if (*s) return 1;

		*s = 1;
	}
	else *s = 0;

	return 0;
}


// Parse a string without interpreting escapes.
static uint8_t* uc_pre_parse_pp_string(uint8_t* p, int32_t d, uc_string_t* s)
{
	int32_t c;

	p++;

	for (;;) 
	{
		c = *p;

		if (c == d) break;
		else if (c == '\\') 
		{
			file->buf_ptr = p;
			c = uc_pre_handle_end_of_buffer();
			p = file->buf_ptr;

			if (c == CH_EOF) 
			{
			LOC_UNTERMINATED_STRING:

				uc_error("Missing terminating '%c' character.", d);
			}
			else if (c == '\\') 
			{
				// Escape: Just skip \[\r]\n.

				PRE_PEEK_C_EOB(c, p);

				if (c == '\n') 
				{
					file->line_num++;
					p++;
				}
				else if (c == '\r') 
				{
					PRE_PEEK_C_EOB(c, p);

					if (c != '\n')
						uc_pre_expected("'\n' after '\r'");

					file->line_num++;
					p++;
				}
				else if (c == CH_EOF) 
					goto LOC_UNTERMINATED_STRING;
				else 
				{
					if (s) 
					{
						uc_pre_string_char_cat(s, '\\');
						uc_pre_string_char_cat(s, c);
					}

					p++;
				}
			}
		}
		else if (c == '\n') 
		{
			file->line_num++;

			goto LOC_ADD_CHARACTER;
		}
		else if (c == '\r') 
		{
			PRE_PEEK_C_EOB(c, p);

			if (c != '\n') 
			{
				if (s)
					uc_pre_string_char_cat(s, '\r');
			}
			else 
			{
				file->line_num++;

				goto LOC_ADD_CHARACTER;
			}
		}
		else 
		{
		LOC_ADD_CHARACTER:

			if (s)
				uc_pre_string_char_cat(s, c);

			p++;
		}
	}

	p++;

	return p;
}


// Skip block of text until #else, #elif or #endif. Also uc_pre_skip pairs of #if / #endif.
static void uc_pre_skip_preprocess(void)
{
	int32_t a = 0, c;
	bool start, abnormal;
	uint8_t* p = file->buf_ptr;

LOC_REDO_START:

	start = true;
	abnormal = false;

	for (;;) 
	{
	LOC_REDO_NO_START:

		c = *p;

		switch (c) 
		{
		case ' ':
		case '\t':
		case '\f':
		case '\v':
		case '\r':

			p++;

			goto LOC_REDO_NO_START;

		case '\n':

			file->line_num++;
			p++;

			goto LOC_REDO_START;

		case '\\':

			file->buf_ptr = p;
			c = uc_pre_handle_end_of_buffer();

			if (c == CH_EOF) 
				uc_pre_expected("#endif");
			else if (c == '\\') 
			{
				character = file->buf_ptr[0];
				uc_pre_handle_stray_no_error();
			}

			p = file->buf_ptr;

			goto LOC_REDO_NO_START;

		// Skip strings.
		case '\"':
		case '\'':

			if (abnormal)
				goto LOC_DEFAULT;

			p = uc_pre_parse_pp_string(p, c, NULL);

			break;

		// Skip comments.
		case '/':

			if (abnormal)
				goto LOC_DEFAULT;

			file->buf_ptr = p;
			character = *p;
			uc_pre_m_in();
			p = file->buf_ptr;

			if (character == '*') 
				p = uc_pre_parse_multiline_comment(p);
			else if (character == '/') 
				p = uc_pre_parse_single_line_comment(p);

			break;

		case '#':

			p++;

			if (start) 
			{
				file->buf_ptr = p;
				uc_pre_next_non_macro();
				p = file->buf_ptr;

				if (a == 0 && (token == TOK_ELSE || token == TOK_ELIF || token == TOK_ENDIF))
					goto LOC_END;

				if (token == TOK_IF || token == TOK_IFDEF || token == TOK_IFNDEF)
					a++;
				else if (token == TOK_ENDIF)
					a--;
				else if (token == TOK_ERROR || token == TOK_WARNING)
					abnormal = true;
				else if (token == TOK_LINEFEED)
					goto LOC_REDO_START;
				else if (parse_flags & UC_OPTION_PARSE_ASSEMBLER)
					p = uc_pre_parse_single_line_comment(p - 1);
			}
			else if (parse_flags & UC_OPTION_PARSE_ASSEMBLER)
				p = uc_pre_parse_single_line_comment(p - 1);

			break;

		LOC_DEFAULT:

		default:
			p++;
			break;
		}

		start = false;
	}

LOC_END:

	file->buf_ptr = p;
}


// Token string handling.
inline static void uc_pre_new_token_string(uc_token_string_t* s)
{
	s->str = NULL;
	s->len = s->lastlen = 0;
	s->allocated_len = 0;
	s->last_line_num = -1;
}


uc_token_string_t* uc_pre_allocate_token_string(void)
{
	uc_token_string_t* s = UC_ALLOCATOR_REALLOC(token_string_allocator, 0, sizeof(uc_token_string_t));
	uc_pre_new_token_string(s);
	return s;
}


int32_t* uc_pre_duplicate_token_string(uc_token_string_t* s)
{
	int32_t* r = UC_ALLOCATOR_REALLOC(token_string_allocator, 0, s->len * sizeof(int32_t));
	memcpy(r, s->str, s->len * sizeof(int32_t));
	return r;
}


void uc_pre_free_token_string_string(int *str)
{
	UC_ALLOCATOR_FREE(token_string_allocator, str);
}


void uc_pre_free_token_string(uc_token_string_t* s)
{
	uc_pre_free_token_string_string(s->str);
	UC_ALLOCATOR_FREE(token_string_allocator, s);
}


int32_t* uc_pre_token_string_realloc(uc_token_string_t* s, int32_t n)
{
	int32_t* r;
	int32_t l = s->allocated_len;

	if (l < 16) l = 16;

	while (l < n) l *= 2;

	if (l > s->allocated_len) 
	{
		r = UC_ALLOCATOR_REALLOC(token_string_allocator, s->str, l * sizeof(int32_t));
		s->allocated_len = l;
		s->str = r;
	}

	return s->str;
}


void uc_pre_token_string_append(uc_token_string_t* s, int32_t c)
{
	int32_t l = s->len;
	int32_t* b = s->str;

	if (l >= s->allocated_len)
		b = uc_pre_token_string_realloc(s, l + 1);

	b[l++] = c;
	s->len = l;
}

void uc_pre_begin_macro(uc_token_string_t* s, int32_t a)
{
	s->alloc = a;
	s->prev = macro_stack;
	s->prev_ptr = macro_pointer;
	s->save_line_num = file->line_num;

	macro_pointer = s->str;
	macro_stack = s;
}


void uc_pre_end_macro(void)
{
	uc_token_string_t* s = macro_stack;

	macro_stack = s->prev;
	macro_pointer = s->prev_ptr;

	file->line_num = s->save_line_num;

	if (s->alloc == 2)
		s->alloc = 3; // Mark as finished.
	else uc_pre_free_token_string(s);
}


static void uc_pre_token_string_add_ex(uc_token_string_t* s, int32_t t, uc_const_value_t* c)
{
	int32_t l = s->lastlen = s->len;
	int32_t* b = s->str;

	// Allocate space for worst case.

	if (l + TOK_MAX_SIZE >= s->allocated_len)
		b = uc_pre_token_string_realloc(s, l + TOK_MAX_SIZE + 1);

	b[l++] = t;

	switch (t) 
	{
	case TOK_CINT:
	case TOK_CUINT:
	case TOK_CCHAR:
	case TOK_LCHAR:
	case TOK_CFLOAT:
	case TOK_LINENUM:

#if LONG_SIZE == 4
	case TOK_CLONG:
	case TOK_CULONG:
#endif

		b[l++] = c->tab[0];
		break;

	case TOK_PPNUM:
	case TOK_PPSTR:
	case TOK_STR:
	case TOK_LSTR:
	{
		// Insert the string into the integer array.

		uint32_t w = 1 + (c->str.size + sizeof(int32_t) - 1) / sizeof(int32_t);

		if (l + w >= s->allocated_len)
			b = uc_pre_token_string_realloc(s, l + w + 1);

		b[l] = c->str.size;
		memcpy(&b[l + 1], c->str.data, c->str.size);
		l += w;

		break;
	}
	
	case TOK_CDOUBLE:
	case TOK_CLLONG:
	case TOK_CULLONG:

#if LONG_SIZE == 8
	case TOK_CLONG:
	case TOK_CULONG:
#endif

#if LDOUBLE_SIZE == 8
	case TOK_CLDOUBLE:
#endif

		b[l++] = c->tab[0];
		b[l++] = c->tab[1];
		break;

#if LDOUBLE_SIZE == 12
	case TOK_CLDOUBLE:
		b[l++] = c->tab[0];
		b[l++] = c->tab[1];
		b[l++] = c->tab[2];
#elif LDOUBLE_SIZE == 16
	case TOK_CLDOUBLE:
		b[l++] = c->tab[0];
		b[l++] = c->tab[1];
		b[l++] = c->tab[2];
		b[l++] = c->tab[3];
#elif LDOUBLE_SIZE != 8
#error TODO: Add long double size support.
#endif

		break;

	default: break;
	}

	s->len = l;
}


// Add the current parse token in the given token string.
void uc_pre_token_string_add_token(uc_token_string_t* s)
{
	uc_const_value_t c;

	// Save line number info.

	if (file->line_num != s->last_line_num) 
	{
		s->last_line_num = file->line_num;
		c.i = s->last_line_num;

		uc_pre_token_string_add_ex(s, TOK_LINENUM, &c);
	}

	uc_pre_token_string_add_ex(s, token, &token_constant);
}


// Get a token from an integer array and increment pointer accordingly.
inline static void __uc_pre_tok_get(int32_t* t, const int32_t** pp, uc_const_value_t* c)
{
	const int32_t* p = *pp;
	int32_t* tab = c->tab;
	int32_t n;

	switch (*t = *p++) 
	{
#if LONG_SIZE == 4
	case TOK_CLONG:
#endif

	case TOK_CINT:
	case TOK_CCHAR:
	case TOK_LCHAR:
	case TOK_LINENUM:
		c->i = *p++;
		break;

#if LONG_SIZE == 4
	case TOK_CULONG:
#endif

	case TOK_CUINT:

		c->i = (unsigned)*p++;
		break;

	case TOK_CFLOAT:

		tab[0] = *p++;
		break;

	case TOK_STR:
	case TOK_LSTR:
	case TOK_PPNUM:
	case TOK_PPSTR:

		c->str.size = *p++;
		c->str.data = p;
		p += (c->str.size + sizeof(int32_t) - 1) / sizeof(int32_t);
		break;

	case TOK_CDOUBLE:
	case TOK_CLLONG:
	case TOK_CULLONG:

#if LONG_SIZE == 8
	case TOK_CLONG:
	case TOK_CULONG:
#endif

		n = 2;
		goto LOC_COPY;

	case TOK_CLDOUBLE:
#if LDOUBLE_SIZE == 16
		n = 4;
#elif LDOUBLE_SIZE == 12
		n = 3;
#elif LDOUBLE_SIZE == 8
		n = 2;
#else
#error add long double size support
#endif

	LOC_COPY:

		do *tab++ = *p++;
		while (--n);
		break;

	default: break;
	}

	*pp = p;
}


static bool uc_pre_macro_is_equal(const int32_t* a, const int32_t* b)
{
	uc_const_value_t c;
	int32_t t;

	if (!a || !b) return true;

	while (*a && *b) 
	{
		// First time preallocate macro resolution buffer, uc_pre_next_expansion time only reset position to start.

		uc_pre_string_reset(&macro_resolution_buffer);

		__uc_pre_tok_get(&t, &a, &c);

		uc_pre_string_cat(&macro_resolution_buffer, uc_get_token_string(t, &c), 0);

		__uc_pre_tok_get(&t, &b, &c);

		if (strcmp(macro_resolution_buffer.data, uc_get_token_string(t, &c)))
			return false;
	}

	return !(*a || *b);
}


// Define handling.


inline static void uc_pre_define_push(int32_t v, int32_t t, int32_t* b, uc_symbol_t* a)
{
	uc_symbol_t* o = uc_pre_define_find(v);
	uc_symbol_t* s = uc_symbol_push_2(&define_stack, v, t, 0);

	s->d = b;
	s->uc_pre_next_expansion = a;

	identifier_table[v - TOK_IDENT]->sym_define = s;

	if (o && !uc_pre_macro_is_equal(o->d, s->d))
		uc_warn("Macro '%s' was redefined.", uc_get_token_string(v, NULL));
}


// Undefine a defined symbol. It's name is just set to null.
void uc_pre_undefine_define(uc_symbol_t* s)
{
	int32_t v = s->v;

	if (v >= TOK_IDENT && v < total_identifiers)
		identifier_table[v - TOK_IDENT]->sym_define = NULL;
}


inline static uc_symbol_t* uc_pre_define_find(int32_t v)
{
	v -= TOK_IDENT;

	if ((unsigned)v >= (unsigned)(total_identifiers - TOK_IDENT))
		return NULL;

	return identifier_table[v]->sym_define;
}


// Free define stack until top reaches 'b'.
void uc_pre_free_definitions_to(uc_symbol_t* b)
{
	while (define_stack != b) 
	{
		uc_symbol_t* t = define_stack;

		define_stack = t->prev;

		uc_pre_free_token_string_string(t->d);
		uc_pre_undefine_define(t);

		uc_symbol_free(t);
	}

	// Restore remaining (-D or predefined) symbols if they were #undef'd in the file.

	while (b) 
	{
		int32_t v = b->v;

		if (v >= TOK_IDENT && v < total_identifiers) 
		{
			uc_symbol_t** d = &identifier_table[v - TOK_IDENT]->sym_define;

			if (!*d) *d = b;
		}

		b = b->prev;
	}
}


// Label lookup.
uc_symbol_t* uc_pre_lookup_label(int32_t v)
{
	v -= TOK_IDENT;

	if ((unsigned)v >= (unsigned)(total_identifiers - TOK_IDENT))
		return NULL;

	return identifier_table[v]->sym_label;
}


uc_symbol_t* uc_pre_push_label(uc_symbol_t** t, int32_t v, int32_t f)
{
	uc_symbol_t* s = uc_symbol_push_2(t, v, 0, 0);
	s->r = f;

	uc_symbol_t** p = &identifier_table[v - TOK_IDENT]->sym_label;

	if (t == &global_label_stack)
	{
		// Modify the top most local identifier, so that symbol identifier will point to 
		// 's' when popped.

		while (*p != NULL)
			p = &(*p)->prev_tok;
	}

	s->prev_tok = *p;
	*p = s;

	return s;
}


// Pop labels until element last is reached. Look if any labels are undefined. 
// Define symbols if '&&label' was used.
void uc_pre_pop_label(uc_symbol_t** t, uc_symbol_t* l, bool k)
{
	uc_symbol_t *s, *p;

	for (s = *t; s != l; s = p) 
	{
		p = s->prev;

		if (s->r == LABEL_DECLARED) 
			uc_warn("Label '%s' is declared but not used.", uc_get_token_string(s->v, NULL));
		else if (s->r == LABEL_FORWARD)
			uc_error("Label '%s' is referenced but not defined.", uc_get_token_string(s->v, NULL));
		else 
		{
			// Define corresponding symbol. A size of 1 is set.

			if (s->c) 
				uc_put_external_symbol(s, cur_text_section, s->jnext, 1);
		}

		// Remove label.
		identifier_table[s->v - TOK_IDENT]->sym_label = s->prev_tok;

		if (!k) uc_symbol_free(s);
	}

	if (!k) *t = l;
}


// Fake the n-th "#if defined test_ ..." for '-dt -run'.
static void uc_pre_perhaps_run_test(uc_state_t* s)
{
	if (s->include_stack_ptr != s->include_stack)
		return;

	const char* p = uc_get_token_string(token, NULL);

	if (memcmp(p, "test_", 5) != 0) return;

	if (0 != --s->run_test) return;

	fprintf(s->ppfp, "\n[%s]\n" + !(s->dflag & 32), p);
	fflush(s->ppfp);

	uc_pre_define_push(token, MACRO_OBJ, NULL, NULL);
}


// Evaluate an expression for #if / #elif.
static bool uc_pre_evaluate_expression(void)
{
	int32_t c, t;

	 uc_token_string_t* s = uc_pre_allocate_token_string();

	preprocessor_expression = 1;

	while (token != TOK_LINEFEED && token != TOK_EOF) 
	{
		uc_pre_next_expansion(); // Do macro substitution.

		if (token == TOK_DEFINED) 
		{
			uc_pre_next_non_macro();

			t = token;

			if (t == '(')
				uc_pre_next_non_macro();

			if (token < TOK_IDENT)
				uc_pre_expected("identifier");

			if (this_state->run_test)
				uc_pre_perhaps_run_test(this_state);

			c = uc_pre_define_find(token) != 0;

			if (t == '(') 
			{
				uc_pre_next_non_macro();

				if (token != ')')
					uc_pre_expected("')'");
			}

			token = TOK_CINT;
			token_constant.i = c;
		}
		else if (token >= TOK_IDENT) 
		{
			// If undefined macro.

			token = TOK_CINT;
			token_constant.i = 0;
		}

		uc_pre_token_string_add_token(s);
	}

	preprocessor_expression = 0;

	uc_pre_token_string_append(s, -1); // Simulate end of file.
	uc_pre_token_string_append(s, 0);

	// Evaluate constant expression.

	uc_pre_begin_macro(s, 1);
	uc_pre_next_expansion();

	c = expr_const();
	uc_pre_end_macro();

	return (c != 0);
}


/* parse after #define */
ST_FUNC void uc_pre_parse_define(void)
{
	uc_symbol_t *s, *first, **ps;
	int v, t, varg, is_vaargs, spc;
	int saved_parse_flags = parse_flags;

	v = token;
	if (v < TOK_IDENT || v == TOK_DEFINED)
		uc_error("invalid macro name '%s'", uc_get_token_string(token, &token_constant));
	/* XXX: should check if same macro (ANSI) */
	first = NULL;
	t = MACRO_OBJ;
	/* We have to parse the whole define as if not in asm mode, in particular
	no line comment with '#' must be ignored. Also for function
	macros the argument list must be parsed without '.' being an ID
	character. */
	parse_flags = ((parse_flags & ~UC_OPTION_PARSE_ASSEMBLER) | UC_OPTION_PARSE_SPACES);
	/* '(' must be just after macro definition for MACRO_FUNC */
	uc_pre_next_non_macro_space();
	if (token == '(') {
		int dotid = uc_pre_set_id_number('.', 0);
		uc_pre_next_non_macro();
		ps = &first;
		if (token != ')') for (;;) {
			varg = token;
			uc_pre_next_non_macro();
			is_vaargs = 0;
			if (varg == TOK_DOTS) {
				varg = TOK___VA_ARGS__;
				is_vaargs = 1;
			}
			else if (token == TOK_DOTS && use_gnu_extensions) {
				is_vaargs = 1;
				uc_pre_next_non_macro();
			}
			if (varg < TOK_IDENT)
				bad_list:
			uc_error("bad macro parameter list");
			s = uc_symbol_push_2(&define_stack, varg | SYM_FIELD, is_vaargs, 0);
			*ps = s;
			ps = &s->uc_pre_next_expansion;
			if (token == ')')
				break;
			if (token != ',' || is_vaargs)
				goto bad_list;
			uc_pre_next_non_macro();
		}
		uc_pre_next_non_macro_space();
		t = MACRO_FUNC;
		uc_pre_set_id_number('.', dotid);
	}

	token_string_buffer.len = 0;
	spc = 2;
	parse_flags |= UC_OPTION_PARSE_ACCEPT_STRAYS | UC_OPTION_PARSE_SPACES | UC_OPTION_PARSE_LINE_FEED;
	/* The body of a macro definition should be parsed such that identifiers
	are parsed like the file mode determines (i.e. with '.' being an
	ID character in asm mode). But '#' should be retained instead of
	regarded as line comment leader, so still don't set ASM_FILE
	in parse_flags. */
	while (token != TOK_LINEFEED && token != TOK_EOF) {
		/* remove spaces around ##and after '#' */
		if (TOK_TWOSHARPS == token) {
			if (2 == spc)
				goto bad_twosharp;
			if (1 == spc)
				--token_string_buffer.len;
			spc = 3;
			token = TOK_PPJOIN;
		}
		else if ('#' == token) {
			spc = 4;
		}
		else if (uc_pre_check_space(token, &spc)) {
			goto uc_pre_skip;
		}
		uc_pre_token_string_add_ex(&token_string_buffer, token, &token_constant);
	uc_pre_skip:
		uc_pre_next_non_macro_space();
	}

	parse_flags = saved_parse_flags;
	if (spc == 1)
		--token_string_buffer.len; /* remove trailing space */
	uc_pre_token_string_append(&token_string_buffer, 0);
	if (3 == spc)
		bad_twosharp:
	uc_error("'##' cannot appear at either end of macro");
	uc_pre_define_push(v, t, uc_pre_duplicate_token_string(&token_string_buffer), first);
}

static uc_cached_include_t *search_cached_include(uc_state_t *s1, const char *filename, int add)
{
	const unsigned char *s;
	unsigned int h;
	uc_cached_include_t *e;
	int i;

	h = UC_PRE_TOKEN_HASH_INITIALIZER;
	s = (uint8_t*)filename;
	while (*s) {
#ifdef _WIN32
		h = UC_PRE_TOKEN_HASH_FUNCTION(h, uc_to_upper(*s));
#else
		h = UC_PRE_TOKEN_HASH_FUNCTION(h, *s);
#endif
		s++;
	}
	h &= (CACHED_INCLUDES_HASH_SIZE - 1);

	i = s1->cached_includes_hash[h];
	for (;;) {
		if (i == 0)
			break;
		e = s1->cached_includes[i - 1];
		if (0 == PATHCMP(e->filename, filename))
			return e;
		i = e->hash_next;
	}
	if (!add)
		return NULL;

	e = uc_malloc(sizeof(uc_cached_include_t) + strlen(filename));
	strcpy(e->filename, filename);
	e->ifndef_macro = e->once = 0;
	dynarray_add(&s1->cached_includes, &s1->nb_cached_includes, e);
	/* add in hash table */
	e->hash_next = s1->cached_includes_hash[h];
	s1->cached_includes_hash[h] = s1->nb_cached_includes;
#ifdef INC_DEBUG
	printf("adding cached '%s'\n", filename);
#endif
	return e;
}

static void pragma_parse(uc_state_t *s1)
{
	uc_pre_next_non_macro();
	if (token == TOK_push_macro || token == TOK_pop_macro) {
		int t = token, v;
		uc_symbol_t *s;

		if (uc_pre_next_expansion(), token != '(')
			goto pragma_err;
		if (uc_pre_next_expansion(), token != TOK_STR)
			goto pragma_err;
		v = uc_pre_token_allocate(token_constant.str.data, token_constant.str.size - 1)->tok;
		if (uc_pre_next_expansion(), token != ')')
			goto pragma_err;
		if (t == TOK_push_macro) {
			while (NULL == (s = uc_pre_define_find(v)))
				uc_pre_define_push(v, 0, NULL, NULL);
			s->type.ref = s; /* set push boundary */
		}
		else {
			for (s = define_stack; s; s = s->prev)
				if (s->v == v && s->type.ref == s) {
					s->type.ref = NULL;
					break;
				}
		}
		if (s)
			identifier_table[v - TOK_IDENT]->sym_define = s->d ? s : NULL;
		else
			uc_warn("unbalanced #pragma pop_macro");
		preprocessor_debug_token = t, preprocessor_debug_symbol = v;

	}
	else if (token == TOK_once) {
		search_cached_include(s1, file->filename, 1)->once = preprocessor_once;

	}
	else if (s1->output_type == TCC_OUTPUT_PREPROCESS) {
		/* tcc -E: keep pragmas below unchanged */
		uc_pre_unget_token(' ');
		uc_pre_unget_token(TOK_PRAGMA);
		uc_pre_unget_token('#');
		uc_pre_unget_token(TOK_LINEFEED);

	}
	else if (token == TOK_pack) {
		/* This may be:
		#pragma pack(1) // set
		#pragma pack() // reset to default
		#pragma pack(push,1) // push & set
		#pragma pack(pop) // restore previous */
		uc_pre_next_expansion();
		uc_pre_skip('(');
		if (token == TOK_ASM_pop) {
			uc_pre_next_expansion();
			if (s1->pack_stack_ptr <= s1->pack_stack) {
			stk_error:
				uc_error("out of pack stack");
			}
			s1->pack_stack_ptr--;
		}
		else {
			int val = 0;
			if (token != ')') {
				if (token == TOK_ASM_push) {
					uc_pre_next_expansion();
					if (s1->pack_stack_ptr >= s1->pack_stack + PACK_STACK_SIZE - 1)
						goto stk_error;
					s1->pack_stack_ptr++;
					uc_pre_skip(',');
				}
				if (token != TOK_CINT)
					goto pragma_err;
				val = token_constant.i;
				if (val < 1 || val > 16 || (val & (val - 1)) != 0)
					goto pragma_err;
				uc_pre_next_expansion();
			}
			*s1->pack_stack_ptr = val;
		}
		if (token != ')')
			goto pragma_err;

	}
	else if (token == TOK_comment) {
		char *p; int t;
		uc_pre_next_expansion();
		uc_pre_skip('(');
		t = token;
		uc_pre_next_expansion();
		uc_pre_skip(',');
		if (token != TOK_STR)
			goto pragma_err;
		p = tcc_strdup((char *)token_constant.str.data);
		uc_pre_next_expansion();
		if (token != ')')
			goto pragma_err;
		if (t == TOK_lib) {
			dynarray_add(&s1->pragma_libs, &s1->nb_pragma_libs, p);
		}
		else {
			if (t == TOK_option)
				tcc_set_options(s1, p);
			uc_free(p);
		}

	}
	else if (s1->warn_unsupported) {
		uc_warn("#pragma %s is ignored", uc_get_token_string(token, &token_constant));
	}
	return;

pragma_err:
	uc_error("malformed #pragma directive");
	return;
}

/* is_bof is true if first non space token at beginning of file */
ST_FUNC void uc_preprocess(int is_bof)
{
	uc_state_t *s1 = this_state;
	int i, c, n, saved_parse_flags;
	char buf[1024], *q;
	uc_symbol_t *s;

	saved_parse_flags = parse_flags;
	parse_flags = UC_OPTION_PARSE_PREPROCESS
		| UC_OPTION_PARSE_TOKEN_NUMBERS
		| UC_OPTION_PARSE_TOKEN_STRINGS
		| UC_OPTION_PARSE_LINE_FEED
		| (parse_flags & UC_OPTION_PARSE_ASSEMBLER)
		;

	uc_pre_next_non_macro();
redo:
	switch (token) {
	case TOK_DEFINE:
		preprocessor_debug_token = token;
		uc_pre_next_non_macro();
		preprocessor_debug_symbol = token;
		uc_pre_parse_define();
		break;
	case TOK_UNDEF:
		preprocessor_debug_token = token;
		uc_pre_next_non_macro();
		preprocessor_debug_symbol = token;
		s = uc_pre_define_find(token);
		/* undefine symbol by putting an invalid name */
		if (s)
			uc_pre_undefine_define(s);
		break;
	case TOK_INCLUDE:
	case TOK_INCLUDE_NEXT:
		character = file->buf_ptr[0];
		/* XXX: incorrect if comments : use uc_pre_next_non_macro with a special mode */
		uc_pre_skip_space();
		if (character == '<') {
			c = '>';
			goto read_name;
		}
		else if (character == '\"') {
			c = character;
		read_name:
			uc_pre_in();
			q = buf;
			while (character != c && character != '\n' && character != CH_EOF) {
				if ((q - buf) < sizeof(buf) - 1)
					*q++ = character;
				if (character == '\\') {
					if (uc_pre_handle_stray_no_error() == 0)
						--q;
				}
				else
					uc_pre_in();
			}
			*q = '\0';
			uc_pre_m_in();
#if 0
			/* eat all spaces and comments after include */
			/* XXX: slightly incorrect */
			while (ch1 != '\n' && ch1 != CH_EOF)
				uc_pre_in();
#endif
		}
		else {
			int len;
			/* computed #include : concatenate everything up to linefeed,
			the result must be one of the two accepted forms.
			Don't convert pp-tokens to tokens here. */
			parse_flags = (UC_OPTION_PARSE_PREPROCESS
				| UC_OPTION_PARSE_LINE_FEED
				| (parse_flags & UC_OPTION_PARSE_ASSEMBLER));
			uc_pre_next_expansion();
			buf[0] = '\0';
			while (token != TOK_LINEFEED) {
				pstrcat(buf, sizeof(buf), uc_get_token_string(token, &token_constant));
				uc_pre_next_expansion();
			}
			len = strlen(buf);
			/* check syntax and remove '<>|""' */
			if ((len < 2 || ((buf[0] != '"' || buf[len - 1] != '"') &&
				(buf[0] != '<' || buf[len - 1] != '>'))))
				uc_error("'#include' expects \"FILENAME\" or <FILENAME>");
			c = buf[len - 1];
			memmove(buf, buf + 1, len - 2);
			buf[len - 2] = '\0';
		}

		if (s1->include_stack_ptr >= s1->include_stack + INCLUDE_STACK_SIZE)
			uc_error("#include recursion too deep");
		/* store current file in stack, but increment stack later below */
		*s1->include_stack_ptr = file;
		i = token == TOK_INCLUDE_NEXT ? file->include_next_index : 0;
		n = 2 + s1->nb_include_paths + s1->nb_sysinclude_paths;
		for (; i < n; ++i) {
			char buf1[sizeof file->filename];
			uc_cached_include_t *e;
			const char *path;

			if (i == 0) {
				/* check absolute include path */
				if (!IS_ABSPATH(buf))
					continue;
				buf1[0] = 0;

			}
			else if (i == 1) {
				/* search in file's dir if "header.h" */
				if (c != '\"')
					continue;
				/* https://savannah.nongnu.org/bugs/index.php?50847 */
				path = file->true_filename;
				pstrncpy(buf1, path, tcc_basename(path) - path);

			}
			else {
				/* search in all the include paths */
				int j = i - 2, k = j - s1->nb_include_paths;
				path = k < 0 ? s1->include_paths[j] : s1->sysinclude_paths[k];
				pstrcpy(buf1, sizeof(buf1), path);
				pstrcat(buf1, sizeof(buf1), "/");
			}

			pstrcat(buf1, sizeof(buf1), buf);
			e = search_cached_include(s1, buf1, 0);
			if (e && (uc_pre_define_find(e->ifndef_macro) || e->once == preprocessor_once)) {
				/* no need to parse the include because the 'ifndef macro'
				is defined (or had #pragma once) */
#ifdef INC_DEBUG
				printf("%s: skipping cached %s\n", file->filename, buf1);
#endif
				goto include_done;
			}

			if (tcc_open(s1, buf1) < 0)
				continue;

			file->include_next_index = i + 1;
#ifdef INC_DEBUG
			printf("%s: including %s\n", file->prev->filename, file->filename);
#endif
			/* update target deps */
			dynarray_add(&s1->target_deps, &s1->nb_target_deps,
				tcc_strdup(buf1));
			/* push current file in stack */
			++s1->include_stack_ptr;
			/* add include file debug info */
			if (s1->do_debug)
				put_stabs(file->filename, N_BINCL, 0, 0, 0);
			token_flags |= UC_TOKEN_FLAG_BOF | UC_TOKEN_FLAG_BOL;
			character = file->buf_ptr[0];
			goto the_end;
		}
		uc_error("include file '%s' not found", buf);
	include_done:
		break;
	case TOK_IFNDEF:
		c = 1;
		goto do_ifdef;
	case TOK_IF:
		c = uc_pre_evaluate_expression();
		goto do_if;
	case TOK_IFDEF:
		c = 0;
	do_ifdef:
		uc_pre_next_non_macro();
		if (token < TOK_IDENT)
			uc_error("invalid argument for '#if%sdef'", c ? "n" : "");
		if (is_bof) {
			if (c) {
#ifdef INC_DEBUG
				printf("#ifndef %s\n", uc_get_token_string(token, NULL));
#endif
				file->ifndef_macro = token;
			}
		}
		c = (uc_pre_define_find(token) != 0) ^ c;
	do_if:
		if (s1->ifdef_stack_ptr >= s1->ifdef_stack + IFDEF_STACK_SIZE)
			uc_error("memory full (ifdef)");
		*s1->ifdef_stack_ptr++ = c;
		goto test_skip;
	case TOK_ELSE:
		if (s1->ifdef_stack_ptr == s1->ifdef_stack)
			uc_error("#else without matching #if");
		if (s1->ifdef_stack_ptr[-1] & 2)
			uc_error("#else after #else");
		c = (s1->ifdef_stack_ptr[-1] ^= 3);
		goto test_else;
	case TOK_ELIF:
		if (s1->ifdef_stack_ptr == s1->ifdef_stack)
			uc_error("#elif without matching #if");
		c = s1->ifdef_stack_ptr[-1];
		if (c > 1)
			uc_error("#elif after #else");
		/* last #if/#elif expression was true: we uc_pre_skip */
		if (c == 1) {
			c = 0;
		}
		else {
			c = uc_pre_evaluate_expression();
			s1->ifdef_stack_ptr[-1] = c;
		}
	test_else:
		if (s1->ifdef_stack_ptr == file->ifdef_stack_ptr + 1)
			file->ifndef_macro = 0;
	test_skip:
		if (!(c & 1)) {
			uc_pre_skip_preprocess();
			is_bof = 0;
			goto redo;
		}
		break;
	case TOK_ENDIF:
		if (s1->ifdef_stack_ptr <= file->ifdef_stack_ptr)
			uc_error("#endif without matching #if");
		s1->ifdef_stack_ptr--;
		/* '#ifndef macro' was at the start of file. Now we check if
		an '#endif' is exactly at the end of file */
		if (file->ifndef_macro &&
			s1->ifdef_stack_ptr == file->ifdef_stack_ptr) {
			file->ifndef_macro_saved = file->ifndef_macro;
			/* need to set to zero to avoid false matches if another
			#ifndef at middle of file */
			file->ifndef_macro = 0;
			while (token != TOK_LINEFEED)
				uc_pre_next_non_macro();
			token_flags |= UC_TOKEN_FLAG_ENDIF;
			goto the_end;
		}
		break;
	case TOK_PPNUM:
		n = strtoul((char*)token_constant.str.data, &q, 10);
		goto _line_num;
	case TOK_LINE:
		uc_pre_next_expansion();
		if (token != TOK_CINT)
			_line_err:
		uc_error("wrong #line format");
		n = token_constant.i;
	_line_num:
		uc_pre_next_expansion();
		if (token != TOK_LINEFEED) {
			if (token == TOK_STR) {
				if (file->true_filename == file->filename)
					file->true_filename = tcc_strdup(file->filename);
				pstrcpy(file->filename, sizeof(file->filename), (char *)token_constant.str.data);
			}
			else if (parse_flags & UC_OPTION_PARSE_ASSEMBLER)
				break;
			else
				goto _line_err;
			--n;
		}
		if (file->fd > 0)
			total_lines += file->line_num - n;
		file->line_num = n;
		if (s1->do_debug)
			put_stabs(file->filename, N_BINCL, 0, 0, 0);
		break;
	case TOK_ERROR:
	case TOK_WARNING:
		c = token;
		character = file->buf_ptr[0];
		uc_pre_skip_space();
		q = buf;
		while (character != '\n' && character != CH_EOF) {
			if ((q - buf) < sizeof(buf) - 1)
				*q++ = character;
			if (character == '\\') {
				if (uc_pre_handle_stray_no_error() == 0)
					--q;
			}
			else
				uc_pre_in();
		}
		*q = '\0';
		if (c == TOK_ERROR)
			uc_error("#error %s", buf);
		else
			uc_warn("#warning %s", buf);
		break;
	case TOK_PRAGMA:
		pragma_parse(s1);
		break;
	case TOK_LINEFEED:
		goto the_end;
	default:
		/* ignore gas line comment in an 'S' file. */
		if (saved_parse_flags & UC_OPTION_PARSE_ASSEMBLER)
			goto ignore;
		if (token == '!' && is_bof)
			/* '!' is ignored at beginning to allow C scripts. */
			goto ignore;
		uc_warn("Ignoring unknown preprocessing directive #%s", uc_get_token_string(token, &token_constant));
	ignore:
		file->buf_ptr = uc_pre_parse_single_line_comment(file->buf_ptr - 1);
		goto the_end;
	}
	/* ignore other uc_preprocess commands or #! for C scripts */
	while (token != TOK_LINEFEED)
		uc_pre_next_non_macro();
the_end:
	parse_flags = saved_parse_flags;
}

/* evaluate escape codes in a string. */
static void parse_escape_string(uc_string_t *outstr, const uint8_t *buf, int is_long)
{
	int c, n;
	const uint8_t *p;

	p = buf;
	for (;;) {
		c = *p;
		if (c == '\0')
			break;
		if (c == '\\') {
			p++;
			/* escape */
			c = *p;
			switch (c) {
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				/* at most three octal digits */
				n = c - '0';
				p++;
				c = *p;
				if (uc_is_octal(c)) {
					n = n * 8 + c - '0';
					p++;
					c = *p;
					if (uc_is_octal(c)) {
						n = n * 8 + c - '0';
						p++;
					}
				}
				c = n;
				goto add_char_nonext;
			case 'x':
			case 'u':
			case 'U':
				p++;
				n = 0;
				for (;;) {
					c = *p;
					if (c >= 'a' && c <= 'f')
						c = c - 'a' + 10;
					else if (c >= 'A' && c <= 'F')
						c = c - 'A' + 10;
					else if (uc_is_number(c))
						c = c - '0';
					else
						break;
					n = n * 16 + c;
					p++;
				}
				c = n;
				goto add_char_nonext;
			case 'a':
				c = '\a';
				break;
			case 'b':
				c = '\b';
				break;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 'v':
				c = '\v';
				break;
			case 'e':
				if (!use_gnu_extensions)
					goto invalid_escape;
				c = 27;
				break;
			case '\'':
			case '\"':
			case '\\':
			case '?':
				break;
			default:
			invalid_escape:
				if (c >= '!' && c <= '~')
					uc_warn("unknown escape sequence: \'\\%c\'", c);
				else
					uc_warn("unknown escape sequence: \'\\x%x\'", c);
				break;
			}
		}
		else if (is_long && c >= 0x80) {
			/* assume we are processing UTF-8 sequence */
			/* reference: The Unicode Standard, Version 10.0, ch3.9 */

			int cont; /* count of continuation bytes */
			int uc_pre_skip; /* how many bytes should uc_pre_skip when error occurred */
			int i;

			/* decode leading byte */
			if (c < 0xC2) {
				uc_pre_skip = 1; goto invalid_utf8_sequence;
			}
			else if (c <= 0xDF) {
				cont = 1; n = c & 0x1f;
			}
			else if (c <= 0xEF) {
				cont = 2; n = c & 0xf;
			}
			else if (c <= 0xF4) {
				cont = 3; n = c & 0x7;
			}
			else {
				uc_pre_skip = 1; goto invalid_utf8_sequence;
			}

			/* decode continuation bytes */
			for (i = 1; i <= cont; ++i) {
				int l = 0x80, h = 0xBF;

				/* adjust limit for second byte */
				if (i == 1) {
					switch (c) {
					case 0xE0: l = 0xA0; break;
					case 0xED: h = 0x9F; break;
					case 0xF0: l = 0x90; break;
					case 0xF4: h = 0x8F; break;
					}
				}

				if (p[i] < l || p[i] > h) {
					uc_pre_skip = i; goto invalid_utf8_sequence;
				}

				n = (n << 6) | (p[i] & 0x3f);
			}

			/* advance pointer */
			p += 1 + cont;
			c = n;
			goto add_char_nonext;

			/* error handling */
		invalid_utf8_sequence:
			uc_warn("ill-formed UTF-8 subsequence starting with: \'\\x%x\'", c);
			c = 0xFFFD;
			p += uc_pre_skip;
			goto add_char_nonext;

		}
		p++;
	add_char_nonext:
		if (!is_long)
			uc_pre_string_char_cat(outstr, c);
		else {
#ifdef TCC_TARGET_PE
			/* store as UTF-16 */
			if (c < 0x10000) {
				uc_pre_string_wchar_cat(outstr, c);
			}
			else {
				c -= 0x10000;
				uc_pre_string_wchar_cat(outstr, (c >> 10) + 0xD800);
				uc_pre_string_wchar_cat(outstr, (c & 0x3FF) + 0xDC00);
			}
#else
			uc_pre_string_wchar_cat(outstr, c);
#endif
		}
	}
	/* add a trailing '\0' */
	if (!is_long)
		uc_pre_string_char_cat(outstr, '\0');
	else
		uc_pre_string_wchar_cat(outstr, '\0');
}

static void parse_string(const char *s, int len)
{
	uint8_t buf[1000], *p = buf;
	int is_long, sep;

	if ((is_long = *s == 'L'))
		++s, --len;
	sep = *s++;
	len -= 2;
	if (len >= sizeof buf)
		p = uc_malloc(len + 1);
	memcpy(p, s, len);
	p[len] = 0;

	uc_pre_string_reset(&current_token);
	parse_escape_string(&current_token, p, is_long);
	if (p != buf)
		uc_free(p);

	if (sep == '\'') {
		int char_size, i, n, c;
		/* XXX: make it portable */
		if (!is_long)
			token = TOK_CCHAR, char_size = 1;
		else
			token = TOK_LCHAR, char_size = sizeof(nwchar_t);
		n = current_token.size / char_size - 1;
		if (n < 1)
			uc_error("empty character constant");
		if (n > 1)
			uc_warn("multi-character character constant");
		for (c = i = 0; i < n; ++i) {
			if (is_long)
				c = ((nwchar_t *)current_token.data)[i];
			else
				c = (c << 8) | ((char *)current_token.data)[i];
		}
		token_constant.i = c;
	}
	else {
		token_constant.str.size = current_token.size;
		token_constant.str.data = current_token.data;
		if (!is_long)
			token = TOK_STR;
		else
			token = TOK_LSTR;
	}
}

/* we use 64 bit numbers */
#define BN_SIZE 2

/* bn = (bn << shift) | or_val */
static void bn_lshift(unsigned int *bn, int shift, int or_val)
{
	int i;
	unsigned int v;
	for (i = 0; i < BN_SIZE; ++i) {
		v = bn[i];
		bn[i] = (v << shift) | or_val;
		or_val = v >> (32 - shift);
	}
}

static void bn_zero(unsigned int *bn)
{
	int i;
	for (i = 0; i < BN_SIZE; ++i) {
		bn[i] = 0;
	}
}

/* parse number in null terminated string 'p' and return it in the
current token */
static void parse_number(const char *p)
{
	int b, t, shift, frac_bits, s, exp_val, character;
	char *q;
	unsigned int bn[BN_SIZE];
	double d;

	/* number */
	q = token_buffer;
	character = *p++;
	t = character;
	character = *p++;
	*q++ = t;
	b = 10;
	if (t == '.') {
		goto float_frac_parse;
	}
	else if (t == '0') {
		if (character == 'x' || character == 'X') {
			q--;
			character = *p++;
			b = 16;
		}
		else if (use_builtin_extensions && (character == 'b' || character == 'B')) {
			q--;
			character = *p++;
			b = 2;
		}
	}
	/* parse all digits. cannot check octal numbers at this stage
	because of floating point constants */
	while (1) {
		if (character >= 'a' && character <= 'f')
			t = character - 'a' + 10;
		else if (character >= 'A' && character <= 'F')
			t = character - 'A' + 10;
		else if (uc_is_number(character))
			t = character - '0';
		else
			break;
		if (t >= b)
			break;
		if (q >= token_buffer + UC_LIMIT_STRING_MAXIMUM_SIZE) {
		num_too_long:
			uc_error("number too long");
		}
		*q++ = character;
		character = *p++;
	}
	if (character == '.' ||
		((character == 'e' || character == 'E') && b == 10) ||
		((character == 'p' || character == 'P') && (b == 16 || b == 2))) {
		if (b != 10) {
			/* NOTE: strtox should support that for hexa numbers, but
			non ISOC99 libcs do not support it, so we prefer to do
			it by hand */
			/* hexadecimal or binary floats */
			/* XXX: handle overflows */
			*q = '\0';
			if (b == 16)
				shift = 4;
			else
				shift = 1;
			bn_zero(bn);
			q = token_buffer;
			while (1) {
				t = *q++;
				if (t == '\0') {
					break;
				}
				else if (t >= 'a') {
					t = t - 'a' + 10;
				}
				else if (t >= 'A') {
					t = t - 'A' + 10;
				}
				else {
					t = t - '0';
				}
				bn_lshift(bn, shift, t);
			}
			frac_bits = 0;
			if (character == '.') {
				character = *p++;
				while (1) {
					t = character;
					if (t >= 'a' && t <= 'f') {
						t = t - 'a' + 10;
					}
					else if (t >= 'A' && t <= 'F') {
						t = t - 'A' + 10;
					}
					else if (t >= '0' && t <= '9') {
						t = t - '0';
					}
					else {
						break;
					}
					if (t >= b)
						uc_error("invalid digit");
					bn_lshift(bn, shift, t);
					frac_bits += shift;
					character = *p++;
				}
			}
			if (character != 'p' && character != 'P')
				uc_pre_expected("exponent");
			character = *p++;
			s = 1;
			exp_val = 0;
			if (character == '+') {
				character = *p++;
			}
			else if (character == '-') {
				s = -1;
				character = *p++;
			}
			if (character < '0' || character > '9')
				uc_pre_expected("exponent digits");
			while (character >= '0' && character <= '9') {
				exp_val = exp_val * 10 + character - '0';
				character = *p++;
			}
			exp_val = exp_val * s;

			/* now we can generate the number */
			/* XXX: should patch directly float number */
			d = (double)bn[1] * 4294967296.0 + (double)bn[0];
			d = ldexp(d, exp_val - frac_bits);
			t = uc_to_upper(character);
			if (t == 'F') {
				character = *p++;
				token = TOK_CFLOAT;
				/* float : should handle overflow */
				token_constant.f = (float)d;
			}
			else if (t == 'L') {
				character = *p++;
#ifdef TCC_TARGET_PE
				token = TOK_CDOUBLE;
				token_constant.d = d;
#else
				token = TOK_CLDOUBLE;
				/* XXX: not large enough */
				token_constant.ld = (long double)d;
#endif
			}
			else {
				token = TOK_CDOUBLE;
				token_constant.d = d;
			}
		}
		else {
			/* decimal floats */
			if (character == '.') {
				if (q >= token_buffer + UC_LIMIT_STRING_MAXIMUM_SIZE)
					goto num_too_long;
				*q++ = character;
				character = *p++;
			float_frac_parse:
				while (character >= '0' && character <= '9') {
					if (q >= token_buffer + UC_LIMIT_STRING_MAXIMUM_SIZE)
						goto num_too_long;
					*q++ = character;
					character = *p++;
				}
			}
			if (character == 'e' || character == 'E') {
				if (q >= token_buffer + UC_LIMIT_STRING_MAXIMUM_SIZE)
					goto num_too_long;
				*q++ = character;
				character = *p++;
				if (character == '-' || character == '+') {
					if (q >= token_buffer + UC_LIMIT_STRING_MAXIMUM_SIZE)
						goto num_too_long;
					*q++ = character;
					character = *p++;
				}
				if (character < '0' || character > '9')
					uc_pre_expected("exponent digits");
				while (character >= '0' && character <= '9') {
					if (q >= token_buffer + UC_LIMIT_STRING_MAXIMUM_SIZE)
						goto num_too_long;
					*q++ = character;
					character = *p++;
				}
			}
			*q = '\0';
			t = uc_to_upper(character);
			errno = 0;
			if (t == 'F') {
				character = *p++;
				token = TOK_CFLOAT;
				token_constant.f = strtof(token_buffer, NULL);
			}
			else if (t == 'L') {
				character = *p++;
#ifdef TCC_TARGET_PE
				token = TOK_CDOUBLE;
				token_constant.d = strtod(token_buffer, NULL);
#else
				token = TOK_CLDOUBLE;
				token_constant.ld = strtold(token_buffer, NULL);
#endif
			}
			else {
				token = TOK_CDOUBLE;
				token_constant.d = strtod(token_buffer, NULL);
			}
		}
	}
	else {
		unsigned long long n, n1;
		int lcount, ucount, ov = 0;
		const char *p1;

		/* integer number */
		*q = '\0';
		q = token_buffer;
		if (b == 10 && *q == '0') {
			b = 8;
			q++;
		}
		n = 0;
		while (1) {
			t = *q++;
			/* no need for checks except for base 10 / 8 errors */
			if (t == '\0')
				break;
			else if (t >= 'a')
				t = t - 'a' + 10;
			else if (t >= 'A')
				t = t - 'A' + 10;
			else
				t = t - '0';
			if (t >= b)
				uc_error("invalid digit");
			n1 = n;
			n = n * b + t;
			/* detect overflow */
			if (n1 >= 0x1000000000000000ULL && n / b != n1)
				ov = 1;
		}

		/* Determine the characteristics (unsigned and/or 64bit) the type of
		the constant must have according to the constant suffix(es) */
		lcount = ucount = 0;
		p1 = p;
		for (;;) {
			t = uc_to_upper(character);
			if (t == 'L') {
				if (lcount >= 2)
					uc_error("three 'l's in integer constant");
				if (lcount && *(p - 1) != character)
					uc_error("incorrect integer suffix: %s", p1);
				lcount++;
				character = *p++;
			}
			else if (t == 'U') {
				if (ucount >= 1)
					uc_error("two 'u's in integer constant");
				ucount++;
				character = *p++;
			}
			else {
				break;
			}
		}

		/* Determine if it needs 64 bits and/or unsigned in order to fit */
		if (ucount == 0 && b == 10) {
			if (lcount <= (LONG_SIZE == 4)) {
				if (n >= 0x80000000U)
					lcount = (LONG_SIZE == 4) + 1;
			}
			if (n >= 0x8000000000000000ULL)
				ov = 1, ucount = 1;
		}
		else {
			if (lcount <= (LONG_SIZE == 4)) {
				if (n >= 0x100000000ULL)
					lcount = (LONG_SIZE == 4) + 1;
				else if (n >= 0x80000000U)
					ucount = 1;
			}
			if (n >= 0x8000000000000000ULL)
				ucount = 1;
		}

		if (ov)
			uc_warn("integer constant overflow");

		token = TOK_CINT;
		if (lcount) {
			token = TOK_CLONG;
			if (lcount == 2)
				token = TOK_CLLONG;
		}
		if (ucount)
			++token; /* TOK_CU... */
		token_constant.i = n;
	}
	if (character)
		uc_error("invalid number\n");
}


#define PARSE2(c1, tok1, c2, tok2) \
case c1: \
PRE_PEEK_C(c, p); \
if (c == c2) { \
p++; \
token = tok2; \
} else { \
token = tok1; \
} \
break;

/* return uc_pre_next_expansion token without macro substitution */
static inline void next_nomacro1(void)
{
	int t, c, is_long, len;
	uc_token_symbol_t *ts;
	uint8_t *p, *p1;
	unsigned int h;

	p = file->buf_ptr;
redo_no_start:
	c = *p;
	switch (c) {
	case ' ':
	case '\t':
		token = c;
		p++;
		if (parse_flags & UC_OPTION_PARSE_SPACES)
			goto keep_tok_flags;
		while (is_id_number_table[*p - CH_EOF] & IS_SPC)
			++p;
		goto redo_no_start;
	case '\f':
	case '\v':
	case '\r':
		p++;
		goto redo_no_start;
	case '\\':
		/* first look if it is in fact an end of buffer */
		c = uc_pre_handle_stray_1(p);
		p = file->buf_ptr;
		if (c == '\\')
			goto parse_simple;
		if (c != CH_EOF)
			goto redo_no_start;
		{
			uc_state_t *s1 = this_state;
			if ((parse_flags & UC_OPTION_PARSE_LINE_FEED)
				&& !(token_flags & UC_TOKEN_FLAG_EOF)) {
				token_flags |= UC_TOKEN_FLAG_EOF;
				token = TOK_LINEFEED;
				goto keep_tok_flags;
			}
			else if (!(parse_flags & UC_OPTION_PARSE_PREPROCESS)) {
				token = TOK_EOF;
			}
			else if (s1->ifdef_stack_ptr != file->ifdef_stack_ptr) {
				uc_error("missing #endif");
			}
			else if (s1->include_stack_ptr == s1->include_stack) {
				/* no include left : end of file. */
				token = TOK_EOF;
			}
			else {
				token_flags &= ~UC_TOKEN_FLAG_EOF;
				/* pop include file */

				/* test if previous '#endif' was after a #ifdef at
				start of file */
				if (token_flags & UC_TOKEN_FLAG_ENDIF) {
#ifdef INC_DEBUG
					printf("#endif %s\n", uc_get_token_string(file->ifndef_macro_saved, NULL));
#endif
					search_cached_include(s1, file->filename, 1)
						->ifndef_macro = file->ifndef_macro_saved;
					token_flags &= ~UC_TOKEN_FLAG_ENDIF;
				}

				/* add end of include file debug info */
				if (this_state->do_debug) {
					put_stabd(N_EINCL, 0, 0);
				}
				/* pop include stack */
				tcc_close();
				s1->include_stack_ptr--;
				p = file->buf_ptr;
				if (p == file->buffer)
					token_flags = UC_TOKEN_FLAG_BOF | UC_TOKEN_FLAG_BOL;
				goto redo_no_start;
			}
		}
		break;

	case '\n':
		file->line_num++;
		token_flags |= UC_TOKEN_FLAG_BOL;
		p++;
	maybe_newline:
		if (0 == (parse_flags & UC_OPTION_PARSE_LINE_FEED))
			goto redo_no_start;
		token = TOK_LINEFEED;
		goto keep_tok_flags;

	case '#':
		/* XXX: simplify */
		PRE_PEEK_C(c, p);
		if ((token_flags & UC_TOKEN_FLAG_BOL) &&
			(parse_flags & UC_OPTION_PARSE_PREPROCESS)) {
			file->buf_ptr = p;
			uc_preprocess(token_flags & UC_TOKEN_FLAG_BOF);
			p = file->buf_ptr;
			goto maybe_newline;
		}
		else {
			if (c == '#') {
				p++;
				token = TOK_TWOSHARPS;
			}
			else {
				if (parse_flags & UC_OPTION_PARSE_ASSEMBLER) {
					p = uc_pre_parse_single_line_comment(p - 1);
					goto redo_no_start;
				}
				else {
					token = '#';
				}
			}
		}
		break;

		/* dollar is allowed to start identifiers when not parsing asm */
	case '$':
		if (!(is_id_number_table[c - CH_EOF] & IS_ID)
			|| (parse_flags & UC_OPTION_PARSE_ASSEMBLER))
			goto parse_simple;

	case 'a': case 'b': case 'c': case 'd':
	case 'e': case 'f': case 'g': case 'h':
	case 'i': case 'j': case 'k': case 'l':
	case 'm': case 'n': case 'o': case 'p':
	case 'q': case 'r': case 's': case 't':
	case 'u': case 'v': case 'w': case 'x':
	case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D':
	case 'E': case 'F': case 'G': case 'H':
	case 'I': case 'J': case 'K':
	case 'M': case 'N': case 'O': case 'P':
	case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X':
	case 'Y': case 'Z':
	case '_':
	parse_ident_fast:
		p1 = p;
		h = UC_PRE_TOKEN_HASH_INITIALIZER;
		h = UC_PRE_TOKEN_HASH_FUNCTION(h, c);
		while (c = *++p, is_id_number_table[c - CH_EOF] & (IS_ID | IS_NUM))
			h = UC_PRE_TOKEN_HASH_FUNCTION(h, c);
		len = p - p1;
		if (c != '\\') {
			uc_token_symbol_t **pts;

			/* fast case : no stray found, so we have the full token
			and we have already hashed it */
			h &= (UC_LIMIT_HASH_SIZE - 1);
			pts = &identifier_hash[h];
			for (;;) {
				ts = *pts;
				if (!ts)
					break;
				if (ts->len == len && !memcmp(ts->str, p1, len))
					goto token_found;
				pts = &(ts->hash_next);
			}
			ts = uc_pre_token_allocate_new(pts, (char *)p1, len);
		token_found:;
		}
		else {
			/* slower case */
			uc_pre_string_reset(&current_token);
			uc_pre_string_cat(&current_token, (char *)p1, len);
			p--;
			PRE_PEEK_C(c, p);
		parse_ident_slow:
			while (is_id_number_table[c - CH_EOF] & (IS_ID | IS_NUM))
			{
				uc_pre_string_char_cat(&current_token, c);
				PRE_PEEK_C(c, p);
			}
			ts = uc_pre_token_allocate(current_token.data, current_token.size);
		}
		token = ts->tok;
		break;
	case 'L':
		t = p[1];
		if (t != '\\' && t != '\'' && t != '\"') {
			/* fast case */
			goto parse_ident_fast;
		}
		else {
			PRE_PEEK_C(c, p);
			if (c == '\'' || c == '\"') {
				is_long = 1;
				goto str_const;
			}
			else {
				uc_pre_string_reset(&current_token);
				uc_pre_string_char_cat(&current_token, 'L');
				goto parse_ident_slow;
			}
		}
		break;

	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
	case '8': case '9':
		t = c;
		PRE_PEEK_C(c, p);
		/* after the first digit, accept digits, alpha, '.' or sign if
		prefixed by 'eEpP' */
	parse_num:
		uc_pre_string_reset(&current_token);
		for (;;) {
			uc_pre_string_char_cat(&current_token, t);
			if (!((is_id_number_table[c - CH_EOF] & (IS_ID | IS_NUM))
				|| c == '.'
				|| ((c == '+' || c == '-')
					&& (((t == 'e' || t == 'E')
						&& !(parse_flags & UC_OPTION_PARSE_ASSEMBLER
							/* 0xe+1 is 3 tokens in asm */
							&& ((char*)current_token.data)[0] == '0'
							&& uc_to_upper(((char*)current_token.data)[1]) == 'X'))
						|| t == 'p' || t == 'P'))))
				break;
			t = c;
			PRE_PEEK_C(c, p);
		}
		/* We add a trailing '\0' to ease parsing */
		uc_pre_string_char_cat(&current_token, '\0');
		token_constant.str.size = current_token.size;
		token_constant.str.data = current_token.data;
		token = TOK_PPNUM;
		break;

	case '.':
		/* special dot handling because it can also start a number */
		PRE_PEEK_C(c, p);
		if (uc_is_number(c)) {
			t = '.';
			goto parse_num;
		}
		else if ((is_id_number_table['.' - CH_EOF] & IS_ID)
			&& (is_id_number_table[c - CH_EOF] & (IS_ID | IS_NUM))) {
			*--p = c = '.';
			goto parse_ident_fast;
		}
		else if (c == '.') {
			PRE_PEEK_C(c, p);
			if (c == '.') {
				p++;
				token = TOK_DOTS;
			}
			else {
				*--p = '.'; /* may underflow into file->unget[] */
				token = '.';
			}
		}
		else {
			token = '.';
		}
		break;
	case '\'':
	case '\"':
		is_long = 0;
	str_const:
		uc_pre_string_reset(&current_token);
		if (is_long)
			uc_pre_string_char_cat(&current_token, 'L');
		uc_pre_string_char_cat(&current_token, c);
		p = uc_pre_parse_pp_string(p, c, &current_token);
		uc_pre_string_char_cat(&current_token, c);
		uc_pre_string_char_cat(&current_token, '\0');
		token_constant.str.size = current_token.size;
		token_constant.str.data = current_token.data;
		token = TOK_PPSTR;
		break;

	case '<':
		PRE_PEEK_C(c, p);
		if (c == '=') {
			p++;
			token = TOK_LE;
		}
		else if (c == '<') {
			PRE_PEEK_C(c, p);
			if (c == '=') {
				p++;
				token = TOK_A_SHL;
			}
			else {
				token = TOK_SHL;
			}
		}
		else {
			token = TOK_LT;
		}
		break;
	case '>':
		PRE_PEEK_C(c, p);
		if (c == '=') {
			p++;
			token = TOK_GE;
		}
		else if (c == '>') {
			PRE_PEEK_C(c, p);
			if (c == '=') {
				p++;
				token = TOK_A_SAR;
			}
			else {
				token = TOK_SAR;
			}
		}
		else {
			token = TOK_GT;
		}
		break;

	case '&':
		PRE_PEEK_C(c, p);
		if (c == '&') {
			p++;
			token = TOK_LAND;
		}
		else if (c == '=') {
			p++;
			token = TOK_A_AND;
		}
		else {
			token = '&';
		}
		break;

	case '|':
		PRE_PEEK_C(c, p);
		if (c == '|') {
			p++;
			token = TOK_LOR;
		}
		else if (c == '=') {
			p++;
			token = TOK_A_OR;
		}
		else {
			token = '|';
		}
		break;

	case '+':
		PRE_PEEK_C(c, p);
		if (c == '+') {
			p++;
			token = TOK_INC;
		}
		else if (c == '=') {
			p++;
			token = TOK_A_ADD;
		}
		else {
			token = '+';
		}
		break;

	case '-':
		PRE_PEEK_C(c, p);
		if (c == '-') {
			p++;
			token = TOK_DEC;
		}
		else if (c == '=') {
			p++;
			token = TOK_A_SUB;
		}
		else if (c == '>') {
			p++;
			token = TOK_ARROW;
		}
		else {
			token = '-';
		}
		break;

		PARSE2('!', '!', '=', TOK_NE)
			PARSE2('=', '=', '=', TOK_EQ)
			PARSE2('*', '*', '=', TOK_A_MUL)
			PARSE2('%', '%', '=', TOK_A_MOD)
			PARSE2('^', '^', '=', TOK_A_XOR)

			/* comments or operator */
	case '/':
		PRE_PEEK_C(c, p);
		if (c == '*') {
			p = uc_pre_parse_multiline_comment(p);
			/* comments replaced by a blank */
			token = ' ';
			goto keep_tok_flags;
		}
		else if (c == '/') {
			p = uc_pre_parse_single_line_comment(p);
			token = ' ';
			goto keep_tok_flags;
		}
		else if (c == '=') {
			p++;
			token = TOK_A_DIV;
		}
		else {
			token = '/';
		}
		break;

		/* simple tokens */
	case '(':
	case ')':
	case '[':
	case ']':
	case '{':
	case '}':
	case ',':
	case ';':
	case ':':
	case '?':
	case '~':
	case '@': /* only used in assembler */
	parse_simple:
		token = c;
		p++;
		break;
	default:
		if (c >= 0x80 && c <= 0xFF) /* utf8 identifiers */
			goto parse_ident_fast;
		if (parse_flags & UC_OPTION_PARSE_ASSEMBLER)
			goto parse_simple;
		uc_error("unrecognized character \\x%02x", c);
		break;
	}
	token_flags = 0;
keep_tok_flags:
	file->buf_ptr = p;
#if defined(UC_OPTION_DEBUG_PARSE)
	printf("token = %d %s\n", token, uc_get_token_string(token, &token_constant));
#endif
}

/* return uc_pre_next_expansion token without macro substitution. Can read input from
macro_pointer buffer */
static void uc_pre_next_non_macro_space(void)
{
	if (macro_pointer) {
	redo:
		token = *macro_pointer;
		if (token) {
			__uc_pre_tok_get(&token, &macro_pointer, &token_constant);
			if (token == TOK_LINENUM) {
				file->line_num = token_constant.i;
				goto redo;
			}
		}
	}
	else {
		next_nomacro1();
	}
	//printf("token = %s\n", uc_get_token_string(token, &token_constant));
}

ST_FUNC void uc_pre_next_non_macro(void)
{
	do {
		uc_pre_next_non_macro_space();
	} while (token < 256 && (is_id_number_table[token - CH_EOF] & IS_SPC));
}


static void macro_subst(
	uc_token_string_t *tok_str,
	uc_symbol_t **nested_list,
	const int *macro_str
);

/* substitute arguments in replacement lists in macro_str by the values in
args (field d) and return allocated string */
static int *macro_arg_subst(uc_symbol_t **nested_list, const int *macro_str, uc_symbol_t *args)
{
	int t, t0, t1, spc;
	const int *st;
	uc_symbol_t *s;
	uc_const_value_t cval;
	uc_token_string_t str;
	uc_string_t cstr;

	uc_pre_new_token_string(&str);
	t0 = t1 = 0;
	while (1) {
		__uc_pre_tok_get(&t, &macro_str, &cval);
		if (!t)
			break;
		if (t == '#') {
			/* stringize */
			__uc_pre_tok_get(&t, &macro_str, &cval);
			if (!t)
				goto bad_stringy;
			s = sym_find2(args, t);
			if (s) {
				uc_pre_string_create(&cstr);
				uc_pre_string_char_cat(&cstr, '\"');
				st = s->d;
				spc = 0;
				while (*st >= 0) {
					__uc_pre_tok_get(&t, &st, &cval);
					if (t != TOK_PLCHLDR
						&& t != TOK_NOSUBST
						&& 0 == uc_pre_check_space(t, &spc)) {
						const char *s = uc_get_token_string(t, &cval);
						while (*s) {
							if (t == TOK_PPSTR && *s != '\'')
								uc_pre_string_add_char(&cstr, *s);
							else
								uc_pre_string_char_cat(&cstr, *s);
							++s;
						}
					}
				}
				cstr.size -= spc;
				uc_pre_string_char_cat(&cstr, '\"');
				uc_pre_string_char_cat(&cstr, '\0');
#ifdef PP_DEBUG
				printf("\nstringize: <%s>\n", (char *)cstr.data);
#endif
				/* add string */
				cval.str.size = cstr.size;
				cval.str.data = cstr.data;
				uc_pre_token_string_add_ex(&str, TOK_PPSTR, &cval);
				uc_pre_string_free(&cstr);
			}
			else {
			bad_stringy:
				uc_pre_expected("macro parameter after '#'");
			}
		}
		else if (t >= TOK_IDENT) {
			s = sym_find2(args, t);
			if (s) {
				int l0 = str.len;
				st = s->d;
				/* if '##' is present before or after, no arg substitution */
				if (*macro_str == TOK_PPJOIN || t1 == TOK_PPJOIN) {
					/* special case for var arg macros : ##eats the ','
					if empty VA_ARGS variable. */
					if (t1 == TOK_PPJOIN && t0 == ',' && use_gnu_extensions && s->type.t) {
						if (*st <= 0) {
							/* suppress ',' '##' */
							str.len -= 2;
						}
						else {
							/* suppress '##' and add variable */
							str.len--;
							goto add_var;
						}
					}
				}
				else {
				add_var:
					if (!s->uc_pre_next_expansion) {
						/* Expand arguments tokens and store them. In most
						cases we could also re-expand each argument if
						used multiple times, but not if the argument
						contains the __COUNTER__ macro. */
						uc_token_string_t str2;
						uc_symbol_push_2(&s->uc_pre_next_expansion, s->v, s->type.t, 0);
						uc_pre_new_token_string(&str2);
						macro_subst(&str2, nested_list, st);
						uc_pre_token_string_append(&str2, 0);
						s->uc_pre_next_expansion->d = str2.str;
					}
					st = s->uc_pre_next_expansion->d;
				}
				for (;;) {
					int t2;
					__uc_pre_tok_get(&t2, &st, &cval);
					if (t2 <= 0)
						break;
					uc_pre_token_string_add_ex(&str, t2, &cval);
				}
				if (str.len == l0) /* expanded to empty string */
					uc_pre_token_string_append(&str, TOK_PLCHLDR);
			}
			else {
				uc_pre_token_string_append(&str, t);
			}
		}
		else {
			uc_pre_token_string_add_ex(&str, t, &cval);
		}
		t0 = t1, t1 = t;
	}
	uc_pre_token_string_append(&str, 0);
	return str.str;
}

static char const ab_month_name[12][4] =
{
"Jan", "Feb", "Mar", "Apr", "May", "Jun",
"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int paste_tokens(int t1, uc_const_value_t *v1, int t2, uc_const_value_t *v2)
{
	uc_string_t cstr;
	int n, ret = 1;

	uc_pre_string_create(&cstr);
	if (t1 != TOK_PLCHLDR)
		uc_pre_string_cat(&cstr, uc_get_token_string(t1, v1), -1);
	n = cstr.size;
	if (t2 != TOK_PLCHLDR)
		uc_pre_string_cat(&cstr, uc_get_token_string(t2, v2), -1);
	uc_pre_string_char_cat(&cstr, '\0');

	uc_open_buffered_file(this_state, ":paste:", cstr.size);
	memcpy(file->buffer, cstr.data, cstr.size);
	token_flags = 0;
	for (;;) {
		next_nomacro1();
		if (0 == *file->buf_ptr)
			break;
		if (uc_is_space(token))
			continue;
		uc_warn("pasting \"%.*s\" and \"%s\" does not give a valid"
			" preprocessing token", n, cstr.data, (char*)cstr.data + n);
		ret = 0;
		break;
	}
	tcc_close();
	//printf("paste <%s>\n", (char*)cstr.data);
	uc_pre_string_free(&cstr);
	return ret;
}

/* handle the '##' operator. Return NULL if no '##' seen. Otherwise
return the resulting string (which must be freed). */
static inline int *macro_twosharps(const int *ptr0)
{
	int t;
	uc_const_value_t cval;
	uc_token_string_t macro_str1;
	int start_of_nosubsts = -1;
	const int *ptr;

	/* we search the first '##' */
	for (ptr = ptr0;;) {
		__uc_pre_tok_get(&t, &ptr, &cval);
		if (t == TOK_PPJOIN)
			break;
		if (t == 0)
			return NULL;
	}

	uc_pre_new_token_string(&macro_str1);

	//uc_pre_print_token(" $$$", ptr0);
	for (ptr = ptr0;;) {
		__uc_pre_tok_get(&t, &ptr, &cval);
		if (t == 0)
			break;
		if (t == TOK_PPJOIN)
			continue;
		while (*ptr == TOK_PPJOIN) {
			int t1; uc_const_value_t cv1;
			/* given 'a##b', remove nosubsts preceding 'a' */
			if (start_of_nosubsts >= 0)
				macro_str1.len = start_of_nosubsts;
			/* given 'a##b', remove nosubsts preceding 'b' */
			while ((t1 = *++ptr) == TOK_NOSUBST)
				;
			if (t1 && t1 != TOK_PPJOIN) {
				__uc_pre_tok_get(&t1, &ptr, &cv1);
				if (t != TOK_PLCHLDR || t1 != TOK_PLCHLDR) {
					if (paste_tokens(t, &cval, t1, &cv1)) {
						t = token, cval = token_constant;
					}
					else {
						uc_pre_token_string_add_ex(&macro_str1, t, &cval);
						t = t1, cval = cv1;
					}
				}
			}
		}
		if (t == TOK_NOSUBST) {
			if (start_of_nosubsts < 0)
				start_of_nosubsts = macro_str1.len;
		}
		else {
			start_of_nosubsts = -1;
		}
		uc_pre_token_string_add_ex(&macro_str1, t, &cval);
	}
	uc_pre_token_string_append(&macro_str1, 0);
	//uc_pre_print_token(" ###", macro_str1.str);
	return macro_str1.str;
}

/* peek or read [ws_str == NULL] uc_pre_next_expansion token from function macro call,
walking up macro levels up to the file if necessary */
static int next_argstream(uc_symbol_t **nested_list, uc_token_string_t *ws_str)
{
	int t;
	const int *p;
	uc_symbol_t *sa;

	for (;;) {
		if (macro_pointer) {
			p = macro_pointer, t = *p;
			if (ws_str) {
				while (uc_is_space(t) || TOK_LINEFEED == t || TOK_PLCHLDR == t)
					uc_pre_token_string_append(ws_str, t), t = *++p;
			}
			if (t == 0) {
				uc_pre_end_macro();
				/* also, end of scope for nested defined symbol */
				sa = *nested_list;
				while (sa && sa->v == 0)
					sa = sa->prev;
				if (sa)
					sa->v = 0;
				continue;
			}
		}
		else {
			character = uc_pre_handle_end_of_buffer();
			if (ws_str) {
				while (uc_is_space(character) || character == '\n' || character == '/') {
					if (character == '/') {
						int c;
						uint8_t *p = file->buf_ptr;
						PRE_PEEK_C(c, p);
						if (c == '*') {
							p = uc_pre_parse_multiline_comment(p);
							file->buf_ptr = p - 1;
						}
						else if (c == '/') {
							p = uc_pre_parse_single_line_comment(p);
							file->buf_ptr = p - 1;
						}
						else
							break;
						character = ' ';
					}
					if (character == '\n')
						file->line_num++;
					if (!(character == '\f' || character == '\v' || character == '\r'))
						uc_pre_token_string_append(ws_str, character);
					uc_pre_m_in();
				}
			}
			t = character;
		}

		if (ws_str)
			return t;
		uc_pre_next_non_macro_space();
		return token;
	}
}

/* do macro substitution of current token with macro 's' and add
result to (tok_str,tok_len). 'nested_list' is the list of all
macros we got inside to avoid recursing. Return non zero if no
substitution needs to be done */
static int uc_pre_macro_substitute_token(
	uc_token_string_t *tok_str,
	uc_symbol_t **nested_list,
	uc_symbol_t *s)
{
	uc_symbol_t *args, *sa, *sa1;
	int parlevel, t, t1, spc;
	uc_token_string_t str;
	char *cstrval;
	uc_const_value_t cval;
	uc_string_t cstr;
	char buf[32];

	/* if symbol is a macro, prepare substitution */
	/* special macros */
	if (token == TOK___LINE__ || token == TOK___COUNTER__) {
		t = token == TOK___LINE__ ? file->line_num : preprocessor_counter++;
		snprintf(buf, sizeof(buf), "%d", t);
		cstrval = buf;
		t1 = TOK_PPNUM;
		goto add_cstr1;
	}
	else if (token == TOK___FILE__) {
		cstrval = file->filename;
		goto add_cstr;
	}
	else if (token == TOK___DATE__ || token == TOK___TIME__) {
		time_t ti;
		struct tm *tm;

		time(&ti);
		tm = localtime(&ti);
		if (token == TOK___DATE__) {
			snprintf(buf, sizeof(buf), "%s %2d %d",
				ab_month_name[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
		}
		else {
			snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
				tm->tm_hour, tm->tm_min, tm->tm_sec);
		}
		cstrval = buf;
	add_cstr:
		t1 = TOK_STR;
	add_cstr1:
		uc_pre_string_create(&cstr);
		uc_pre_string_cat(&cstr, cstrval, 0);
		cval.str.size = cstr.size;
		cval.str.data = cstr.data;
		uc_pre_token_string_add_ex(tok_str, t1, &cval);
		uc_pre_string_free(&cstr);
	}
	else if (s->d) {
		int saved_parse_flags = parse_flags;
		int *joined_str = NULL;
		int *mstr = s->d;

		if (s->type.t == MACRO_FUNC) {
			/* whitespace between macro name and argument list */
			uc_token_string_t ws_str;
			uc_pre_new_token_string(&ws_str);

			spc = 0;
			parse_flags |= UC_OPTION_PARSE_SPACES | UC_OPTION_PARSE_LINE_FEED
				| UC_OPTION_PARSE_ACCEPT_STRAYS;

			/* get uc_pre_next_expansion token from argument stream */
			t = next_argstream(nested_list, &ws_str);
			if (t != '(') {
				/* not a macro substitution after all, restore the
				* macro token plus all whitespace we've read.
				* whitespace is intentionally not merged to preserve
				* newlines. */
				parse_flags = saved_parse_flags;
				uc_pre_token_string_append(tok_str, token);
				if (parse_flags & UC_OPTION_PARSE_SPACES) {
					int i;
					for (i = 0; i < ws_str.len; ++i)
						uc_pre_token_string_append(tok_str, ws_str.str[i]);
				}
				uc_pre_free_token_string_string(ws_str.str);
				return 0;
			}
			else {
				uc_pre_free_token_string_string(ws_str.str);
			}
			do {
				uc_pre_next_non_macro(); /* eat '(' */
			} while (token == TOK_PLCHLDR);

			/* argument macro */
			args = NULL;
			sa = s->uc_pre_next_expansion;
			/* NOTE: empty args are allowed, except if no args */
			for (;;) {
				do {
					next_argstream(nested_list, NULL);
				} while (uc_is_space(token) || TOK_LINEFEED == token);
			empty_arg:
				/* handle '()' case */
				if (!args && !sa && token == ')')
					break;
				if (!sa)
					uc_error("macro '%s' used with too many args",
						uc_get_token_string(s->v, 0));
				uc_pre_new_token_string(&str);
				parlevel = spc = 0;
				/* NOTE: non zero sa->t indicates VA_ARGS */
				while ((parlevel > 0 ||
					(token != ')' &&
					(token != ',' || sa->type.t)))) {
					if (token == TOK_EOF || token == 0)
						break;
					if (token == '(')
						parlevel++;
					else if (token == ')')
						parlevel--;
					if (token == TOK_LINEFEED)
						token = ' ';
					if (!uc_pre_check_space(token, &spc))
						uc_pre_token_string_add_ex(&str, token, &token_constant);
					next_argstream(nested_list, NULL);
				}
				if (parlevel)
					uc_pre_expected(")");
				str.len -= spc;
				uc_pre_token_string_append(&str, -1);
				uc_pre_token_string_append(&str, 0);
				sa1 = uc_symbol_push_2(&args, sa->v & ~SYM_FIELD, sa->type.t, 0);
				sa1->d = str.str;
				sa = sa->uc_pre_next_expansion;
				if (token == ')') {
					/* special case for gcc var args: add an empty
					var arg argument if it is omitted */
					if (sa && sa->type.t && use_gnu_extensions)
						goto empty_arg;
					break;
				}
				if (token != ',')
					uc_pre_expected(",");
			}
			if (sa) {
				uc_error("macro '%s' used with too few args",
					uc_get_token_string(s->v, 0));
			}

			parse_flags = saved_parse_flags;

			/* now subst each arg */
			mstr = macro_arg_subst(nested_list, mstr, args);
			/* free memory */
			sa = args;
			while (sa) {
				sa1 = sa->prev;
				uc_pre_free_token_string_string(sa->d);
				if (sa->uc_pre_next_expansion) {
					uc_pre_free_token_string_string(sa->uc_pre_next_expansion->d);
					uc_symbol_free(sa->uc_pre_next_expansion);
				}
				uc_symbol_free(sa);
				sa = sa1;
			}
		}

		uc_symbol_push_2(nested_list, s->v, 0, 0);
		parse_flags = saved_parse_flags;
		joined_str = macro_twosharps(mstr);
		macro_subst(tok_str, nested_list, joined_str ? joined_str : mstr);

		/* pop nested defined symbol */
		sa1 = *nested_list;
		*nested_list = sa1->prev;
		uc_symbol_free(sa1);
		if (joined_str)
			uc_pre_free_token_string_string(joined_str);
		if (mstr != s->d)
			uc_pre_free_token_string_string(mstr);
	}
	return 0;
}

/* do macro substitution of macro_str and add result to
(tok_str,tok_len). 'nested_list' is the list of all macros we got
inside to avoid recursing. */
static void macro_subst(
	uc_token_string_t *tok_str,
	uc_symbol_t **nested_list,
	const int *macro_str
)
{
	uc_symbol_t *s;
	int t, spc, nosubst;
	uc_const_value_t cval;

	spc = nosubst = 0;

	while (1) {
		__uc_pre_tok_get(&t, &macro_str, &cval);
		if (t <= 0)
			break;

		if (t >= TOK_IDENT && 0 == nosubst) {
			s = uc_pre_define_find(t);
			if (s == NULL)
				goto no_subst;

			/* if nested substitution, do nothing */
			if (sym_find2(*nested_list, t)) {
				/* and mark it as TOK_NOSUBST, so it doesn't get subst'd again */
				uc_pre_token_string_add_ex(tok_str, TOK_NOSUBST, NULL);
				goto no_subst;
			}

			{
				uc_token_string_t str;
				str.str = (int*)macro_str;
				uc_pre_begin_macro(&str, 2);

				token = t;
				uc_pre_macro_substitute_token(tok_str, nested_list, s);

				if (str.alloc == 3) {
					/* already finished by reading function macro arguments */
					break;
				}

				macro_str = macro_pointer;
				uc_pre_end_macro();
			}
			if (tok_str->len)
				spc = uc_is_space(t = tok_str->str[tok_str->lastlen]);
		}
		else {
			if (t == '\\' && !(parse_flags & UC_OPTION_PARSE_ACCEPT_STRAYS))
				uc_error("stray '\\' in program");
		no_subst:
			if (!uc_pre_check_space(t, &spc))
				uc_pre_token_string_add_ex(tok_str, t, &cval);

			if (nosubst) {
				if (nosubst > 1 && (spc || (++nosubst == 3 && t == '(')))
					continue;
				nosubst = 0;
			}
			if (t == TOK_NOSUBST)
				nosubst = 1;
		}
		/* GCC supports 'defined' as result of a macro substitution */
		if (t == TOK_DEFINED && preprocessor_expression)
			nosubst = 2;
	}
}


// Return uc_pre_next_expansion token with macro expansion.
void uc_pre_next_expansion(void)
{

LOC_REDO:

	if (parse_flags & UC_OPTION_PARSE_SPACES)
		uc_pre_next_non_macro_space();
	else uc_pre_next_non_macro();

	if (macro_pointer) 
	{
		if (token == TOK_NOSUBST || token == TOK_PLCHLDR) 
		{
			// Discard preprocessor markers.

			goto LOC_REDO;
		}
		else if (token == 0) 
		{
			// End of macro or un-get token string.

			uc_pre_end_macro();

			goto LOC_REDO;
		}
	}
	else if (token >= TOK_IDENT && (parse_flags & UC_OPTION_PARSE_PREPROCESS)) 
	{
		// If reading from file, try to substitute macros.

		uc_symbol_t* s = uc_pre_define_find(token);

		if (s) 
		{
			uc_symbol_t* n = NULL;

			token_string_buffer.len = 0;

			uc_pre_macro_substitute_token(&token_string_buffer, &n, s);
			uc_pre_token_string_append(&token_string_buffer, 0);
			uc_pre_begin_macro(&token_string_buffer, 2);

			goto LOC_REDO;
		}
	}

	// Convert preprocessor tokens into C tokens.

	if (token == TOK_PPNUM) 
	{
		if (parse_flags & UC_OPTION_PARSE_TOKEN_NUMBERS)
			parse_number((char *)token_constant.str.data);
	}
	else if (token == TOK_PPSTR) 
	{
		if (parse_flags & UC_OPTION_PARSE_TOKEN_STRINGS)
			parse_string((char *)token_constant.str.data, token_constant.str.size - 1);
	}
}


// Push back current token and set current token to 'l'. Only identifier case handled for labels.
inline static void uc_pre_unget_token(int32_t l)
{
	uc_token_string_t* s = uc_pre_allocate_token_string();

	uc_pre_token_string_add_ex(s, token, &token_constant);
	uc_pre_token_string_append(s, 0);
	uc_pre_begin_macro(s, 1);

	token = l;
}


void uc_pre_preprocess_start(uc_state_t* state, bool assembler)
{
	uc_string_t s;
	int32_t i;

	state->include_stack_ptr = state->include_stack;
	state->ifdef_stack_ptr = state->ifdef_stack;

	file->ifdef_stack_ptr = state->ifdef_stack_ptr;

	preprocessor_expression = 0;
	preprocessor_counter = 0;
	preprocessor_debug_token = preprocessor_debug_symbol = 0;
	preprocessor_once++;

	pvtop = vtop = vstack - 1;

	state->pack_stack[0] = 0;
	state->pack_stack_ptr = state->pack_stack;

	uc_pre_set_id_number('$', state->dollars_in_identifiers ? IS_ID : 0);
	uc_pre_set_id_number('.', assembler ? IS_ID : 0);

	uc_pre_string_create(&s);
	uc_pre_string_cat(&s, "\"", -1);
	uc_pre_string_cat(&s, file->filename, -1);
	uc_pre_string_cat(&s, "\"", 0);

	uc_define_symbol(state, "__BASE_FILE__", s.data);

	uc_pre_string_reset(&s);

	for (i = 0; i < state->nb_cmd_include_files; ++i)
	{
		uc_pre_string_cat(&s, "#include \"", -1);
		uc_pre_string_cat(&s, state->cmd_include_files[i], -1);
		uc_pre_string_cat(&s, "\"\n", -1);
	}

	if (s.size)
	{
		*state->include_stack_ptr++ = file;

		uc_open_buffered_file(state, "<command line>", s.size);

		memcpy(file->buffer, s.data, s.size);
	}

	uc_pre_string_free(&s);

	if (assembler)
		uc_define_symbol(state, "__ASSEMBLER__", NULL);

	parse_flags = assembler ? UC_OPTION_PARSE_ASSEMBLER : 0;
	token_flags = UC_TOKEN_FLAG_BOL | UC_TOKEN_FLAG_BOF;
}


// Cleanup from error/setjmp.
void uc_pre_preprocess_end(uc_state_t* s)
{
	while (macro_stack)
		uc_pre_end_macro();

	macro_pointer = NULL;
}


void uc_create_preprocessor(uc_state_t* s)
{
	int i, c;
	const char *p, *r;

	// Might be used in error() before uc_pre_preprocess_start().

	s->include_stack_ptr = s->include_stack;
	s->ppfp = stdout;

	// Init is-id table.

	for (i = CH_EOF; i < 128; ++i)
		uc_pre_set_id_number(i, uc_is_space(i) ? IS_SPC : uc_is_id(i) ? IS_ID : uc_is_number(i) ? IS_NUM : 0);

	for (i = 128; i < 256; ++i)
		uc_pre_set_id_number(i, IS_ID);

	// Init allocators.

	uc_pre_allocator_create(&token_symbol_allocator, UC_ALLOCATOR_TOKEN_SYMBOL_LIMIT, UC_ALLOCATOR_TOKEN_SYMBOL_SIZE);
	uc_pre_allocator_create(&token_string_allocator, UC_ALLOCATOR_TOKEN_STRING_LIMIT, UC_ALLOCATOR_TOKEN_STRING_SIZE);
	uc_pre_allocator_create(&string_allocator, UC_ALLOCATOR_STRING_LIMIT, UC_ALLOCATOR_STRING_SIZE);

	memset(identifier_hash, 0, UC_LIMIT_HASH_SIZE * sizeof(uc_token_symbol_t*));

	uc_pre_string_create(&string_buffer);
	uc_pre_string_realloc(&string_buffer, UC_LIMIT_STRING_MAXIMUM_SIZE);
	uc_pre_new_token_string(&token_string_buffer);
	uc_pre_token_string_realloc(&token_string_buffer, UC_LIMIT_TOKEN_STRING_MAXIMUM_SIZE);

	total_identifiers = TOK_IDENT;
	p = uc_pre_key_words_table;

	while (*p) 
	{
		r = p;

		for (;;) 
		{
			c = *r++;

			if (c == '\0')
				break;
		}

		uc_pre_token_allocate(p, r - p - 1);

		p = r;
	}
}


void uc_destroy_preprocessor(uc_state_t* s)
{
	int32_t i, n;

	// Free '-D' and compiler defines.

	uc_pre_free_definitions_to(NULL);

	// Free tokens.

	n = total_identifiers - TOK_IDENT;

	for (i = 0; i < n; ++i)
		UC_ALLOCATOR_FREE(token_symbol_allocator, identifier_table[i]);

	uc_free(identifier_table);
	identifier_table = NULL;

	// Free buffers.

	uc_pre_string_free(&current_token);
	uc_pre_string_free(&string_buffer);
	uc_pre_string_free(&macro_resolution_buffer);
	uc_pre_free_token_string_string(token_string_buffer.str);

	// Free allocators.

	uc_pre_allocator_destroy(token_symbol_allocator);
	token_symbol_allocator = NULL;

	uc_pre_allocator_destroy(token_string_allocator);
	token_string_allocator = NULL;

	uc_pre_allocator_destroy(string_allocator);
	string_allocator = NULL;
}


// Support for '-E [-P[1]] [-dD}'.
static void uc_pre_print_token(const char* message, const int32_t* value)
{
	int32_t t, s = 0;
	uc_const_value_t c;

	FILE* f = this_state->ppfp;

	fprintf(f, "%s", message);

	while (value)
	{
		__uc_pre_tok_get(&t, &value, &c);

		if (!t) break;

		fprintf(f, " %s" + s, uc_get_token_string(t, &c));

		s = 1;
	}

	fprintf(f, "\n");
	fflush(f);
}


static void uc_pre_output_line(uc_state_t* s, uc_buffered_file_t* f, int32_t l)
{
	int32_t d = f->line_num - f->line_ref;

	if (s->dflag & 4)
		return;

	if (s->p_flag == LINE_MACRO_OUTPUT_FORMAT_NONE) 
	{
		;
	}
	else if (l == 0 && f->line_ref && d < 8)
	{
		while (d > 0)
		{
			fputs("\n", s->ppfp);
			--d;
		}
	}
	else if (s->p_flag == LINE_MACRO_OUTPUT_FORMAT_STD) 
	{
		fprintf(s->ppfp, "#line %d \"%s\"\n", f->line_num, f->filename);
	}
	else {
		fprintf(s->ppfp, "#%d \"%s\"%s\n", f->line_num, f->filename, l > 0 ? " 1" : l < 0 ? " 2" : "");
	}

	f->line_ref = f->line_num;
}


static void uc_pre_print_define(uc_state_t* s, int32_t v)
{
	uc_symbol_t* d = uc_pre_define_find(v);

	if (d == NULL || d->d == NULL)
		return;

	FILE* f = s->ppfp;

	fprintf(f, "#define %s", uc_get_token_string(v, NULL));

	if (d->type.t == MACRO_FUNC) 
	{
		uc_symbol_t* a = d->uc_pre_next_expansion;

		fprintf(f, "(");

		if (a)
		{
			for (;;) 
			{
				fprintf(f, "%s", uc_get_token_string(a->v & ~SYM_FIELD, NULL));

				if (!(a = a->uc_pre_next_expansion))
					break;

				fprintf(f, ",");
			}
		}

		fprintf(f, ")");
	}

	uc_pre_print_token("", d->d);
}


static void uc_pre_debug_defines(uc_state_t* s)
{
	int32_t t = preprocessor_debug_token;

	if (t == 0)
		return;

	file->line_num--;

	uc_pre_output_line(s, file, 0);

	file->line_ref = ++file->line_num;

	FILE* f = s->ppfp;
	int32_t v = preprocessor_debug_symbol;
	const char* k = uc_get_token_string(v, NULL);

	if (t == TOK_DEFINE)
		uc_pre_print_define(s, v);
	else if (t == TOK_UNDEF)
		fprintf(f, "#undef %s\n", k);
	else if (t == TOK_push_macro)
		fprintf(f, "#pragma push_macro(\"%s\")\n", k);
	else if (t == TOK_pop_macro)
		fprintf(f, "#pragma pop_macro(\"%s\")\n", k);
	
	preprocessor_debug_token = 0;
}


static void uc_pre_debug_builtins(uc_state_t* s)
{
	int32_t v;

	for (v = TOK_IDENT; v < total_identifiers; ++v)
		uc_pre_print_define(s, v);
}


// Need to add a space between tokens a and b to avoid unwanted textual pasting.
static bool uc_pre_need_space(int32_t a, int32_t b)
{
	return 'E' == a ? '+' == b || '-' == b : 
		'+' == a ? TOK_INC == b || '+' == b : 
		'-' == a ? TOK_DEC == b || '-' == b : 
		a >= TOK_IDENT ? b >= TOK_IDENT : 
		a == TOK_PPNUM ? b >= TOK_IDENT : 0;
}


// Maybe hex like 0x1e.
static int32_t uc_pre_check_hex_e(int32_t t, const char* p)
{
	if (t == TOK_PPNUM && uc_to_upper(strchr(p, 0)[-1]) == 'E')
		return 'E';
	return t;
}


// Preprocess the current file.
int32_t uc_pre_preprocess(uc_state_t* s)
{
	uc_buffered_file_t** b;
	const char* p;
	char w[512];

	parse_flags = UC_OPTION_PARSE_PREPROCESS
		| (parse_flags & UC_OPTION_PARSE_ASSEMBLER)
		| UC_OPTION_PARSE_LINE_FEED
		| UC_OPTION_PARSE_SPACES
		| UC_OPTION_PARSE_ACCEPT_STRAYS
		;

	if (s->p_flag == LINE_MACRO_OUTPUT_FORMAT_P10)
		parse_flags |= UC_OPTION_PARSE_TOKEN_NUMBERS, s->p_flag = 1;

	if (s->dflag & 1)
	{
		uc_pre_debug_builtins(s);

		s->dflag &= ~1;
	}

	int32_t o = TOK_LINEFEED;
	int32_t i = 0;

	uc_pre_output_line(s, file, 0);

	for (;;) 
	{
		b = s->include_stack_ptr;

		uc_pre_next_expansion();

		if (token == TOK_EOF)
			break;

		int32_t l = s->include_stack_ptr - b;

		if (l) 
		{
			if (l > 0)
				uc_pre_output_line(s, *b, 0);

			uc_pre_output_line(s, file, l);
		}

		if (s->dflag & 7) 
		{
			uc_pre_debug_defines(s);

			if (s->dflag & 4)
				continue;
		}

		if (uc_is_space(token)) 
		{
			if (i < sizeof w - 1)
				w[i++] = token;

			continue;
		}
		else if (token == TOK_LINEFEED) 
		{
			i = 0;

			if (o == TOK_LINEFEED)
				continue;

			++file->line_ref;
		}
		else if (o == TOK_LINEFEED) 
		{
			uc_pre_output_line(s, file, 0);
		}
		else if (i == 0 && uc_pre_need_space(o, token)) 
		{
			w[i++] = ' ';
		}

		w[i] = 0;
		fputs(w, s->ppfp);
		i = 0;

		fputs(p = uc_get_token_string(token, &token_constant), s->ppfp);

		o = uc_pre_check_hex_e(token, p);
	}

	return 0;
}



/*
	mkc.c
	Makes machine code definitions for inclusion in the secure_memory project.
	This contains the main() driver for the mkc (MaKe Code) utility.
	I tried to make it zero-dependency.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <float.h>
#include <fenv.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/timeb.h>
#include <sys/stat.h>


#include "../config.h"
#include "../compatibility/gettimeofday.h"
#include "../bits.h"
#include "../sm.h"


#if defined(SM_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN 1
#include <time.h>
#include <process.h>
#include <windows.h>
#define getpid _getpid
#define time _time64
#elif defined(SM_OS_LINUX)
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif


static uint8_t sm_have_rdrand[] = { 0x53, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x0F, 0xA2, 0x0F, 0xBA, 0xE1, 0x1E, 0x73, 0x09, 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x5B, 0xC3, 0x48, 0x2B, 0xC0, 0xEB, 0xF2 };
static uint8_t sm_rdrand[] = { 0x48, 0x0F, 0xC7, 0xF0, 0x73, 0xFA, 0xC3 };


#include "crc.h"
;

extern void callconv ran_f_seed(void *restrict s, uint64_t seed);
extern uint64_t callconv ran_f_rand(void *restrict s);

//extern void* callconv sm_transcode(uint8_t encode, void *restrict data, register size_t bytes, uint64_t key, void* state, size_t size, void(*seed)(void*, uint64_t), uint64_t(*random)(void*));
extern void* callconv sm_xor_pass(void *restrict data, register size_t bytes, uint64_t key);
extern uint64_t callconv crc_64(uint64_t c, register const uint8_t *restrict p, uint64_t n, void* t);


uint64_t next_rand()
{
	uint8_t i, n = 1 + (1 + rand()) % 32;
	union { uint32_t i[2]; uint64_t q; } u1, u2;

	u1.i[0] = (uint32_t)rand();
	u1.i[1] = (uint32_t)rand();

	for (i = 0; i < n; ++i)
	{
		if (i & 1) u1.i[0] ^= (uint32_t)rand();
		else u1.i[1] ^= (uint32_t)rand();
		u1.q = sm_rotl_64(u1.q, i + 1);
	}

	u2.i[0] = (uint32_t)rand();
	u2.i[1] = (uint32_t)rand();

	return sm_shuffle_64(u1.q) ^ sm_yellow_64(u2.q);
}


// Delete the specified file before opening it.
FILE* open_file(const char* path)
{
	FILE* r = fopen(path, "r");
	if (r) fclose(r);
	if (r) remove(path);
	return fopen(path, "w");
}


// Trim leading and following whitespace.
char* trim(char* s)
{
	while (isspace((unsigned char)*s)) s++;
	if (*s == 0) return s;
	char* e = s + strlen(s) - 1;
	while (e > s && isspace((int)*e)) e--;
	*(e + 1) = 0;
	return s;
}


// Replace m in s with r.
int replace(char* s, const char* m, const char* r)
{
	int rc = 0;
	char b[0x4000] = { 0 };
	char* i = &b[0];
	const char *p, *t = s;
	size_t ml = strlen(m), rl = strlen(r);
	while (1) 
	{
		p = strstr(t, m);
		if (p == NULL) 
		{
			strcpy(i, t);
			break;
		}
		memcpy(i, t, p - t);
		i += p - t;
		memcpy(i, r, rl);
		i += rl;
		t = p + ml;
		rc++;
	}
	strcpy(s, b);
	return rc;
}


int evaluate_size(char* entity_size, uint64_t* result);


/*
Input Format: 

For a data entry:

# Optional emitted comment.\n
D: NAME = BYTE0 .. BYTEN;\n

For a general function entry:

# Optional emitted comment.\n
F: NAME = BYTE0 .. BYTEN;\n

For an intrinsic function entry:

# Optional emitted comment.\n
I: NAME = BYTE0 .. BYTEN;\n

For a size entry:

# Optional emitted comment.\n
S: NAME = EXPR;\n

*/


// Makes an 8-character hex alias name, where the initial char is an hex letter digit.
char* make_alias(char* buffer)
{
	const char prefixes[6] = { 'a', 'b', 'c', 'd', 'e', 'f' };
	uint64_t code1 = next_rand();
	uint64_t code2 = next_rand();
	sprintf(buffer, "%c%08" PRIx64 "%08" PRIx64, prefixes[(next_rand() + 1) % sizeof(prefixes)], code1, code2);
	buffer[16] = '\0';
	return buffer;
}


#define KIND_DATA 1 // A data entry.
#define KIND_FUNCTION 2 // A protected function entry.
#define KIND_INTEGRAL 3 // An integral/bootstrap function entry.
#define KIND_SIZE 4 // A size entry.


// Readable descriptions of entities
const char* get_descriptor(int kind)
{
	static const char* data = "Protected Data";
	static const char* function = "Protected Function";
	static const char* integral = "Bootstrap/Integral Function";
	static const char* size = "Size";
	static const char* unknown = "Unknown";

	switch (kind)
	{
	case KIND_DATA: return data;
	case KIND_FUNCTION: return function;
	case KIND_INTEGRAL: return integral;
	case KIND_SIZE: return size;
	default: return unknown;
	}
}


// The entity tags used in making names for elements.
const char* get_entity_tag(int kind)
{
	static const char* data = "data";
	static const char* function = "function";
	static const char* integral = "integral";
	static const char* size = "size";
	static const char* unknown = "unknown";

	switch (kind)
	{
	case KIND_DATA: return data;
	case KIND_FUNCTION: return function;
	case KIND_INTEGRAL: return integral;
	case KIND_SIZE: return size;
	default: return unknown;
	}
}


void output_crc_tabs();
void generate_mutator_function(const char* name);

uint64_t callconv foo(uint64_t value);


int main(int argc, char* argv[])
{
	printf("MKC Code Generator for Secure Memory Library\n");
	printf("Copyright (C) 2017-2018 by the Secure Memory Project, All rights Reserved\n");

	srand(((uint32_t)time(NULL)) ^ ((uint32_t)getpid()));

	uint64_t v0 = next_rand();
	//uint64_t v1 = foo(v0);
	//printf("%016" PRIX64 " -> %016" PRIX64 "\n", v0, v1);

	if (argc == 2 && strlen(argv[1]) > 3 && strncmp(argv[1], "-crc", 4) == 0)
	{
		output_crc_tabs();
		return 0;
	}

	if (argc == 3 && strlen(argv[1]) > 3 && strncmp(argv[1], "-mut", 4) == 0)
	{
		generate_mutator_function(argv[2]);
		return 0;
	}

	if (argc < 3)
	{
		printf("USAGE: mkc <stem> <in0> [.. inN]");
		return -1;
	}

	char* target_stem = argv[1];

	char target_op_decl_name[512], target_op_data_name[512], target_op_impl_name[512];
	FILE *target_op_decl_file, *target_op_data_file, *target_op_impl_file, *input;

	sprintf(target_op_decl_name, "%s_decl.h", target_stem);
	sprintf(target_op_data_name, "%s_data.h", target_stem);
	sprintf(target_op_impl_name, "%s_impl.h", target_stem);

	target_op_decl_file = open_file(target_op_decl_name);
	target_op_data_file = open_file(target_op_data_name);
	target_op_impl_file = open_file(target_op_impl_name);

	if (!target_op_decl_file) { printf("mkc: Error: Failed to open decl file \"%s\" for output.\n", target_op_decl_name); return -1; }
	if (!target_op_data_file) { printf("mkc: Error: Failed to open data file \"%s\" for output.\n", target_op_data_name); return -1; }
	if (!target_op_impl_file) { printf("mkc: Error: Failed to open impl file \"%s\" for output.\n", target_op_impl_name); return -1; }

	time_t clk;
	time(&clk);
	struct tm* ctm = localtime(&clk);

	char now[512];
	strcpy(now, asctime(ctm));
	now[strlen(now) - 1] = '\0';

	if (replace(now, "  ", " ") > 0)
		replace(now, "  ", " ");

	fprintf(target_op_decl_file, "// %s - Auto-Generated (%s): Declarations for the '%s' module. Include in your module header file.\n\n", target_op_decl_name, now, target_stem);
	fprintf(target_op_data_file, "// %s - Auto-Generated (%s): Data for the '%s' module. Include in your module source file.\n\n", target_op_data_name, now, target_stem);
	fprintf(target_op_impl_file, "// %s - Auto-Generated (%s): Switch entries for the '%s' module. Include in the sm_get_entity() function switch statement, in your module source file.\n\n", target_op_impl_name, now, target_stem);

	uint64_t entity_xor_key = 0, entity_alias_id = 0;
	uint16_t entity_op_code = 0xFFFF;

	char entity_alias_name[128];

	uint32_t i, j;

	for (i = 2; i < (uint32_t)argc; ++i)
	{
		input = fopen(argv[i], "r");

		if (input == NULL)
		{
			printf("mkc: Warning: Failed to open source file \"%s\" for input. Skipping.\n", argv[i]);
			continue;
		}

		char line_buffer[0x4000] = { 0 }, *line;
		char comment[1024] = { 0 };

		int kind = 0;

		while (fgets(line_buffer, 0x4000, input))
		{
			line = trim(line_buffer);

			if (strlen(line) == 0) continue;

			line[0] = toupper(line[0]);

			if (line[0] == '#')
			{
				line = trim(line + 1);
				if (strlen(line) == 0) comment[0] = '\0';
				else sprintf(comment, "%s", trim(line));
			}
			else if (strlen(line) > 12 && line[0] == 'D' || line[0] == 'F' || line[0] == 'S' || line[0] == 'I')
			{
				switch (line[0])
				{
				case 'D': kind = KIND_DATA; break;
				case 'F': kind = KIND_FUNCTION; break;
				case 'I': kind = KIND_INTEGRAL; break;
				case 'S': kind = KIND_SIZE; break;
				default: break;
				}

				char* working = trim(line + 2);
				char* tok = strtok(working, " :,=;");

				char entity_name[128];
				char entity_name_upper[128];
				char entity_size[1024];
				strcpy(entity_name, tok);

				uint32_t entity_code_len = 0;
				uint8_t entity_bytes[0x4000];

				if (kind < KIND_SIZE)
				{
					while ((tok = strtok(NULL, " \t:,=;")) != NULL)
					{
						char* eptr = NULL;
						entity_bytes[entity_code_len] = (uint8_t)strtoul(tok, &eptr, 16);
						entity_code_len++;
					}

					if (entity_code_len == 0)
					{
						comment[0] = '\0';
						line_buffer[0] = '\0';
						entity_size[0] = '\0';
						continue;
					}
				}
				else
				{
					tok = strtok(NULL, "=;");

					if (tok == NULL)
					{
						comment[0] = '\0';
						line_buffer[0] = '\0';
						entity_size[0] = '\0';
						continue;
					}

					tok = trim(tok);

					if (strlen(tok) == 0)
					{
						comment[0] = '\0';
						line_buffer[0] = '\0';
						entity_size[0] = '\0';
						continue;
					}

					strcpy(entity_size, tok);
				}

				entity_xor_key = next_rand();
				entity_alias_id = next_rand();

				entity_op_code = 0;
				while (entity_op_code < 0x100)
					entity_op_code = UINT16_C(0xFF) + 1 + (next_rand() % UINT64_C(0xFEFE));

				strcpy(entity_name_upper, entity_name);
				strupr(entity_name_upper);

				if (kind < KIND_SIZE)
				{
					printf("mkc: Generating %d code bytes for \"%s\" (%s type)...\n", entity_code_len, entity_name, get_descriptor(kind));

					uint64_t entity_crc_64 = crc_64(entity_xor_key, entity_bytes, entity_code_len, (void*)crc_64_tab);

					//if (kind == KIND_INTEGRAL) 
						sm_xor_pass(entity_bytes, (size_t)entity_code_len, entity_xor_key);
					//else sm_transcode(1, entity_bytes, (size_t)entity_code_len, entity_xor_key, random_state, sizeof(random_state), ran_f_seed, ran_f_rand);

					if (strlen(comment))
						fprintf(target_op_data_file, "// %s (%s - opcode 0x%04X): %s\n", get_descriptor(kind), entity_name_upper, entity_op_code, comment);
					else fprintf(target_op_data_file, "// %s (%s - opcode 0x%04X).\n", get_descriptor(kind), entity_name_upper, entity_op_code);

					fprintf(target_op_data_file, "#define %s_key %s\n", entity_name, make_alias(entity_alias_name));
					fprintf(target_op_data_file, "#define %s_size %s\n", entity_name, make_alias(entity_alias_name));
					fprintf(target_op_data_file, "#define %s_crc %s\n", entity_name, make_alias(entity_alias_name));
					fprintf(target_op_data_file, "#define %s_data %s\n", entity_name, make_alias(entity_alias_name));

					fprintf(target_op_data_file, "static uint64_t %s_key = UINT64_C(0x%" PRIX64 ");\n", entity_name, entity_xor_key);
					fprintf(target_op_data_file, "static uint64_t %s_size = UINT64_C(0x%" PRIX64 ");\n", entity_name, ((uint64_t)entity_code_len) * sizeof(uint8_t));
					fprintf(target_op_data_file, "static uint64_t %s_crc = UINT64_C(0x%" PRIX64 ");\n", entity_name, entity_crc_64);
					fprintf(target_op_data_file, "static uint8_t %s_data[] = {\n\t", entity_name);

					for (j = 0; j < entity_code_len; ++j)
					{
						fprintf(target_op_data_file, "0x%02X", entity_bytes[j]);
						if (j < (entity_code_len - 1)) fprintf(target_op_data_file, ", ");
						if (j && j % 16 == 0) fprintf(target_op_data_file, "\n\t");
					}

					fprintf(target_op_data_file, " };\n\n");
				}
				else
				{
					uint64_t entity_size_value = 0ULL;

					if (evaluate_size(entity_size, &entity_size_value) >= 0)
						sprintf(entity_size, "0x%" PRIX64 "U", entity_size_value);
					else
					{
						printf("Error: %s: Failed to evaluate flag or size expression \"%s\" (condensed).\n", entity_name, entity_size);
						return -1;
					}

					if (entity_size_value == 0)
						printf("Warning: %s: Entity flag or size expression \"%s\" (condensed) evaluates to zero.\n", entity_name, entity_size);
				}

				if (strlen(comment))
					fprintf(target_op_decl_file, "#define SM_GET_%s (0x%04XU) // %s: %s\n", entity_name_upper, entity_op_code, get_descriptor(kind), comment);
				else fprintf(target_op_decl_file, "#define SM_GET_%s (0x%04XU) // %s.\n", entity_name_upper, entity_op_code, get_descriptor(kind));

				if (kind < KIND_SIZE)
					fprintf(target_op_impl_file, "case SM_GET_%s: return (uint64_t)sm_load_entity(context, %d, %s_data, %s_size, &%s_key, &%s_crc);\n", entity_name_upper, 
						(kind == KIND_FUNCTION || kind == KIND_INTEGRAL) ? 1 : 0, entity_name, entity_name, entity_name, entity_name);
				else 
				{
					if (strlen(comment))
						fprintf(target_op_data_file, "#define SM_GET_%s %s // %s: %s\n\n", entity_name, entity_size, get_descriptor(kind), comment);
					else fprintf(target_op_data_file, "#define SM_GET_%s %s // %s.\n\n", entity_name, entity_size, get_descriptor(kind));

					fprintf(target_op_impl_file, "case SM_GET_%s: return %s;\n", entity_name_upper, entity_name);
				}

				comment[0] = '\0';
				line_buffer[0] = '\0';
				entity_size[0] = '\0';
				entity_name[0] = '\0';
				entity_name_upper[0] = '\0';
			}

			line_buffer[0] = '\0';
		}

		fclose(input);

		input = NULL;
	}

	fflush(target_op_decl_file);
	fclose(target_op_decl_file);

	fflush(target_op_data_file);
	fclose(target_op_data_file);

	fflush(target_op_impl_file);
	fclose(target_op_impl_file);

	printf("mkc: Generated: %s_decl.h, %s_data.h, and %s_impl.h.\n", target_stem, target_stem, target_stem);
	printf("mkc: Done.\n");

	return 0;
}


void output_crc_tabs()
{
	uint64_t ent_code_len = sizeof(crc_64_tab);
	uint8_t* ent_bytes = (void*)crc_64_tab;

	uint64_t j;

	fprintf(stdout, "# CRC 64-Bit LUT.\n");
	fprintf(stdout, "DA: crc_64_tab = ");

	for (j = 0; j < ent_code_len; ++j)
	{
		fprintf(stdout, "%02X", ent_bytes[j]);
		if (j < (ent_code_len - 1)) fprintf(stdout, " ");
	}

	fprintf(stdout, ";\n\n");

	ent_code_len = sizeof(crc_32_tab);
	ent_bytes = (void*)crc_32_tab;

	fprintf(stdout, "# CRC 32-Bit LUT.\n");
	fprintf(stdout, "DA: crc_32_tab = ");

	for (j = 0; j < ent_code_len; ++j)
	{
		fprintf(stdout, "%02X", ent_bytes[j]);
		if (j < (ent_code_len - 1)) fprintf(stdout, " ");
	}

	fprintf(stdout, ";\n\n");
}


char* to_size_string(char* buf, uint64_t size)
{
	sprintf(buf, "%" PRIu64, size);
	return buf;
}


int eval(char*, uint64_t*);


// Evaluate size expressions.
int evaluate_size(char* entity_size, uint64_t* result)
{
	uint32_t i = 0, n, b = 10, q = 0;
	char buf[256], *d;

	entity_size = trim(entity_size);

	// Eliminate multi-pointer expressions since they are likely of the same size.

	while (replace(entity_size, "**)", "*)"));

	// We're using single-char operator tokens.

	replace(entity_size, ">>", ">");
	replace(entity_size, "<<", "<");

#define REP(X) replace(entity_size, #X, to_size_string(buf, X))

	// Speculatively replace sizeof expressions with their values.

	// PODs & their pointers.

	REP(sizeof(char));                   REP(sizeof(char*));
	REP(sizeof(unsigned char));          REP(sizeof(unsigned char*));
	REP(sizeof(short));                  REP(sizeof(short*));
	REP(sizeof(short int));              REP(sizeof(short int*));
	REP(sizeof(unsigned short));         REP(sizeof(unsigned short*));
	REP(sizeof(unsigned short int));     REP(sizeof(unsigned short int*));
	REP(sizeof(int));                    REP(sizeof(int*));
	REP(sizeof(unsigned int));           REP(sizeof(unsigned int*));
	REP(sizeof(long));                   REP(sizeof(long*));
	REP(sizeof(unsigned long));          REP(sizeof(unsigned long*));
	REP(sizeof(unsigned long int));      REP(sizeof(unsigned long int*));
	REP(sizeof(long long));              REP(sizeof(long long*));
	REP(sizeof(long long int));          REP(sizeof(long long int*));
	REP(sizeof(unsigned long long));     REP(sizeof(unsigned long long*));
	REP(sizeof(unsigned long long int)); REP(sizeof(unsigned long long int*));
	REP(sizeof(float));                  REP(sizeof(float*));
	REP(sizeof(double));                 REP(sizeof(double*));
	REP(sizeof(long double));            REP(sizeof(long double*));
	REP(sizeof(void*));

	// Well-know types & their pointers.

	REP(sizeof(int8_t));				REP(sizeof(int8_t*));
	REP(sizeof(int16_t));   			REP(sizeof(int16_t*));
	REP(sizeof(int32_t));   			REP(sizeof(int32_t*));
	REP(sizeof(int64_t));   			REP(sizeof(int64_t*));
	REP(sizeof(uint8_t));   			REP(sizeof(uint8_t*));
	REP(sizeof(uint16_t));  			REP(sizeof(uint16_t*));
	REP(sizeof(uint32_t));  			REP(sizeof(uint32_t*));
	REP(sizeof(uint64_t));  			REP(sizeof(uint64_t*));
	REP(sizeof(size_t));    			REP(sizeof(size_t*));
	REP(sizeof(ssize_t));   			REP(sizeof(ssize_t*));
	REP(sizeof(off_t));     			REP(sizeof(off_t*));
	REP(sizeof(ptrdiff_t)); 			REP(sizeof(ptrdiff_t*));
	REP(sizeof(clock_t)); 				REP(sizeof(clock_t*));
	REP(sizeof(time_t)); 				REP(sizeof(time_t*));
	REP(sizeof(wchar_t)); 				REP(sizeof(wchar_t*));
	REP(sizeof(errno_t));				REP(sizeof(errno_t*));
	REP(sizeof(uintptr_t));				REP(sizeof(uintptr_t*));
	REP(sizeof(uid_t));					REP(sizeof(uid_t*));
	REP(sizeof(pid_t));					REP(sizeof(pid_t*));
	REP(sizeof(div_t));					REP(sizeof(div_t*));
	REP(sizeof(fexcept_t));				REP(sizeof(fexcept_t*));
	REP(sizeof(fpos_t));				REP(sizeof(fpos_t*));
	REP(sizeof(locale_t));				REP(sizeof(locale_t*));
	REP(sizeof(struct timeval));		REP(sizeof(struct timeval*));
	REP(sizeof(struct tm));				REP(sizeof(struct tm*));
	REP(sizeof(struct timeb));			REP(sizeof(struct timeb*));
	REP(sizeof(struct stat));			REP(sizeof(struct stat*));
	REP(sizeof(fd_set));				REP(sizeof(fd_set*));
	REP(sizeof(FILE*)); 				

#if defined(SM_OS_WINDOWS)
	REP(sizeof(HANDLE));
	REP(sizeof(GUID));					REP(sizeof(GUID*));
#else
	REP(sizeof(guid_t));				REP(sizeof(guid_t*));
#endif

	// Constants from limits.h, stdint.h, and float.h.

	REP(CHAR_BIT);
	REP(SCHAR_MAX);						REP(UCHAR_MAX);
	REP(SHRT_MAX);						REP(USHRT_MAX);
	REP(INT_MAX);						REP(UINT_MAX);
	REP(LONG_MAX);						REP(ULONG_MAX);
	REP(LLONG_MAX);						REP(ULLONG_MAX);
	REP(INT8_MAX);						REP(UINT8_MAX);
	REP(INT16_MAX);						REP(UINT16_MAX);
	REP(INT32_MAX);						REP(UINT32_MAX);
	REP(INT64_MAX);						REP(UINT64_MAX);
	REP(FLT_MANT_DIG);					REP(FLT_MAX_EXP);
	REP(DBL_MANT_DIG);					REP(DBL_MAX_EXP);
	REP(LDBL_MANT_DIG);					REP(LDBL_MAX_EXP);

#undef REP

	// Test for expressions of a single numerical value with no parens or operators.

	n = (uint32_t)strlen(entity_size);

	if (n > 2 && entity_size[0] == '0' && entity_size[1] == 'x')
	{
		entity_size += 2;
		b = 16;
	}

	q = 1;

	for (i = 0; i < n; ++i) // Look for non-numeric characters.
		if (!isxdigit(entity_size[i])) { q = 0; break; }

	// Just one number, so we don't need to evaluate.

	if (q)
	{
		*result = strtoull(entity_size, &d, b);
		return 0;
	}

	*result = 0;

	return eval(entity_size, result);
}


// Dynamic evaluation support.
static char sym[] = "+-*/^<>|&%#)(", ops[256], tok[256];
static int opp, asp, par, sta = 0;
static uint64_t arg[256];static void opsh(char), apsh(uint64_t);
static int apop(uint64_t*), opop(int*), oeva(), peva();
static char *eget(char*), *oget(char*);


// Primitive unsigned 64-bit expression evaluator. Evaluates str and returns the computed value in *val. Returns 0 on success, else -1.
// Supports the following infix operators: +, -, *, /, < (left-shift), > (right-shift), |, &, ^, %, and # (exponentiation).
static int eval(char* str, uint64_t* val)
{
	int r = 0;
	uint64_t a;
	char *p, *s, *e, *t = str;
	for (; *t; ++t) { p = t; while (*p && isspace(*p)) ++p; if (t != p) strcpy(t, p); }
	p = str;
	*val = 0;

	while (*p)
	{
		if (sta == 0)
		{
			if (NULL != (s = eget(p)))
			{
				if ('(' == *s) { opsh(*s), p += strlen(s); continue; }
				if (s[0] == '0' && s[1] == 'x') a = strtoull(s + 2, &e, 16);
				else a = strtoull(s, &e, 10);
				apsh(a);
				p += strlen(s);
			}
			else return -1;
			sta = 1;
		}
		else
		{
			if (NULL == (s = oget(p))) return -1;
			if (strchr(sym, *s))
			{
				if (')' == *s) { if (0 > (r = peva())) return r; }
				else opsh(*s), sta = 0;
				p += strlen(s);
			}
			else return -1;
		}
	}

	while (1 < asp) 
		if (0 > (r = oeva())) 
			return r;

	if (!opp) return apop(val);
	else return -1;
}


static int peva() { int o; if (1 > par--) return -1; do if (0 > (o = oeva())) break; while ('(' != o); return o; }
static void opsh(char o) { if ('(' == o) ++par; ops[opp++] = o; }
static void apsh(uint64_t a) { arg[asp++] = a; }
static int apop(uint64_t* a) { *a = arg[--asp]; return (0 > asp) ? -1 : 0; }
static int opop(int* o) { if (!opp) return -1; *o = ops[--opp]; return 0; }
static char* oget(char* s) { *tok = *s; tok[1] = '\0'; return tok; }


static char* eget(char* s)
{
	char *p = s, *t = tok;
	while (*p)
	{
		if (strchr(sym, *p))
		{
			if (s == p) return oget(s);
			else break;
		}
		*t++ = *p++;
	}
	*t = '\0';
	return tok;
}


// Unsigned exponentiation.
static uint64_t upow(uint64_t b, uint64_t e)
{
	uint64_t r = UINT64_C(1);
	while (e)
	{
		if (e & UINT64_C(1)) r *= b;
		e >>= 1;
		b *= b;
	}
	return r;
}


// Eval operators.
static int oeva()
{
	uint64_t a, b;
	int o;
	if (opop(&o) == -1) return -1;
	apop(&a);
	apop(&b);
	switch (o)
	{
	case '+': apsh(b + a); break;
	case '-': apsh(b - a); break;
	case '*': apsh(b * a); break;
	case '/': if (a && b) apsh(b / a); else return -1; break;
	case '<': apsh(b << a); break;
	case '>': apsh(b >> a); break;
	case '|': apsh(b | a); break;
	case '&': apsh(b & a); break;
	case '^': apsh(b ^ a); break;
	case '%':  if (a && b) apsh(b % a); else return -1; break;
	case '#':  apsh(upow(b, a)); break;
	case '(': asp += 2; break;
	default: return -1;
	}
	if (asp < 1) return -1;
	return o;
}


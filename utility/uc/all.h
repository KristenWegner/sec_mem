// all.h


#ifndef INCLUDE_ALL_H
#define INCLUDE_ALL_H 1


#include "cfg.h"


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <setjmp.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>


#ifndef PUB_FUNC // Mark functions used by compiler.
#define PUB_FUNC
#endif


#define ST_INLN
#define ST_FUNC
#define ST_DATA extern


#ifndef _WIN32

#include <unistd.h>
#include <sys/time.h>

#ifndef CONFIG_TCC_STATIC
#include <dlfcn.h>
#endif

extern float strtof(const char *__nptr, char **__endptr);
extern long double strtold(const char *__nptr, char **__endptr);

#endif // !_WIN32


#ifdef _WIN32


#include <windows.h>
#include <io.h>
#include <direct.h>


#define inline __inline
#define snprintf _snprintf
#define vsnprintf _vsnprintf


#ifndef __GNUC__
#define strtold strtold
#define strtof strtof
#define strtoll _strtoi64
#define strtoull _strtoui64
#endif

#ifdef LIBTCC_AS_DLL
#define LIBTCCAPI __declspec(dllexport)
#define PUB_FUNC LIBTCCAPI
#endif


#define inp next_inp // Intrinsic on Windows.


#ifdef _MSC_VER
#pragma warning(disable : 4244) // Conversion from 'uint64_t' to 'int', possible loss of data.
#pragma warning(disable : 4267) // Conversion from 'size_t' to 'int', possible loss of data.
#pragma warning(disable : 4996) // The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name.
#pragma warning(disable : 4018) // Signed / unsigned mismatch.
#pragma warning(disable : 4146) // Unary minus operator applied to unsigned type, result still unsigned.
#define ssize_t intptr_t
#endif


#undef CONFIG_TCC_STATIC


#endif // _WIN32


#ifndef O_BINARY
#define O_BINARY 0
#endif


#ifndef offsetof
#define offsetof(T, F) ((size_t)&((T*)0)->F)
#endif


#ifndef countof
#define countof(A) (sizeof(A) / sizeof((A)[0]))
#endif


#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#define ALIGNED(x) __declspec(align(x))
#else
#define NORETURN __attribute__((noreturn))
#define ALIGNED(x) __attribute__((aligned(x)))
#endif


// Default target is x86.


#if !defined(TCC_TARGET_I386) && !defined(TCC_TARGET_ARM) && !defined(TCC_TARGET_ARM64) && !defined(TCC_TARGET_X86_64)
#if defined __x86_64__ || defined _AMD64_
#define TCC_TARGET_X86_64
#elif defined __arm__
#define TCC_TARGET_ARM
#define TCC_ARM_EABI
#define TCC_ARM_HARDFLOAT
#elif defined __aarch64__
#define TCC_TARGET_ARM64
#else
#define TCC_TARGET_I386
#endif
#ifdef _WIN32
#define TCC_TARGET_PE 1
#endif
#endif


// Only native compiler supports '-run'.


#if defined(_WIN32) && defined(TCC_TARGET_PE)
#if (defined(__i386__) || defined(_X86_)) && defined(TCC_TARGET_I386)
#define TCC_IS_NATIVE 1
#elif (defined(__x86_64__) || defined(_AMD64_)) && defined(TCC_TARGET_X86_64)
#define TCC_IS_NATIVE 1
#elif defined(__arm__) && defined(TCC_TARGET_ARM)
#define TCC_IS_NATIVE 1
#elif defined(__aarch64__) && defined(TCC_TARGET_ARM64)
#define TCC_IS_NATIVE 1
#endif
#endif


#if defined TCC_IS_NATIVE && !defined CONFIG_TCCBOOT
#define CONFIG_TCC_BACKTRACE
#if (defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64) && !defined TCC_UCLIBC && !defined TCC_MUSL
#define CONFIG_TCC_BCHECK // Enable bound checking code.
#endif
#endif


#ifdef CONFIG_TCC_ELFINTERP
#define DEFAULT_ELFINTERP(s) CONFIG_TCC_ELFINTERP
#else
#define DEFAULT_ELFINTERP(s) default_elfinterp(s)
#endif


#include "elf.h"
#include "dbx.h"


#ifdef TCC_PROFILE // Profile all functions.
#define static
#endif


#if PTR_SIZE == 8
#define ELFCLASSW ELFCLASS64
#define ElfW(T) Elf##64##_##T
#define ELFW(T) ELF##64##_##T
#define ElfW_Rel ElfW(Rela)
#define SHT_RELX SHT_RELA
#define REL_SECTION_FMT ".rela%s"
#else
#define ELFCLASSW ELFCLASS32
#define ElfW(T) Elf##32##_##T
#define ELFW(T) ELF##32##_##T
#define ElfW_Rel ElfW(Rel)
#define SHT_RELX SHT_REL
#define REL_SECTION_FMT ".rel%s"
#endif


// Target address type.


#define addr_t ElfW(Addr)
#define ElfSym ElfW(Sym)


#if PTR_SIZE == 8 && !defined(TCC_TARGET_PE)
#define LONG_SIZE 8
#else
#define LONG_SIZE 4
#endif


// Token symbol management.
typedef struct uc_token_symbol_s 
{
    struct uc_token_symbol_s* hash_next;

    struct uc_symbol_s* sym_define; // Direct pointer to define.
    struct uc_symbol_s* sym_label; // Direct pointer to label.
    struct uc_symbol_s* sym_struct; // Direct pointer to structure.
    struct uc_symbol_s* sym_identifier; // Direct pointer to identifier.

    int32_t tok; // Token number.
	int32_t len;

    char str[1];
}
uc_token_symbol_t;


#ifdef TCC_TARGET_PE
typedef uint16_t nwchar_t;
#else
typedef int32_t nwchar_t;
#endif


typedef struct uc_string_s 
{
    int32_t size; // Size in bytes.
    void* data; // Either 'char*' or 'nwchar_t*'.
	int32_t size_allocated;
}
uc_string_t;


// Type definition.
typedef struct uc_c_type_s 
{
	int32_t t;
    struct uc_symbol_s* ref;
}
uc_c_type_t;


// Constant value.
typedef union uc_const_value_u 
{
    long double ld;
    double d;
    float f;
    uint64_t i;

    struct 
	{
		int32_t size;
        const void* data;
    } 
	str;

	int32_t tab[LDOUBLE_SIZE / 4];
}
uc_const_value_t;


// Value on stack.
typedef struct uc_stack_value_s
{
	uc_c_type_t type; // Type.
	uint16_t r; // Register + flags.
	uint16_t r2; // Second register, used for 'long long' type. If not used, set to VT_CONST.
	uc_const_value_t c; // Constant, if VT_CONST.
	struct uc_symbol_s* sym; // Symbol, if (VT_SYM | VT_CONST), or if result of unary() for an identifier.
}
uc_stack_value_t;


// Symbol attributes.
typedef struct uc_symbol_attributes_s 
{
	uint16_t
    aligned     : 5, // Alignment as log2 + 1 (0 == unspecified).
    packed      : 1,
    weak        : 1,
    visibility  : 2,
    dllexport   : 1,
    dllimport   : 1,
    unused      : 5;
}
uc_symbol_attributes_t;


// Function attributes or temporary attributes for parsing.
typedef struct uc_function_attributes_s 
{
	uint32_t
    func_call : 3, // Calling convention (0 .. 5), see below.
    func_type : 2, // FUNC_OLD / new / ellipsis.
    func_args : 8; // PE '__stdcall' args.
}
uc_function_attributes_t;


// GNU C attribute definition.
typedef struct uc_attribute_definition_s 
{
    uc_symbol_attributes_t a;
    uc_function_attributes_t f;
    struct uc_section_s* section;
	int32_t alias_target; // Token.
	int32_t asm_label; // Associated assembler label.
    char attr_mode; // __attribute__((__mode__(...))).
}
uc_attribute_definition_t;


// Symbol management.
typedef struct uc_symbol_s
{
	int32_t v; // Symbol token.
    uint16_t r; // Associated register or VT_CONST/VT_LOCAL and L-value type.
    uc_symbol_attributes_t a; // Symbol attributes.

    union 
	{
        struct 
		{
			int32_t c; // Associated number or ELF symbol index.

            union 
			{
				int32_t sym_scope; // Scope level for locals.
				int32_t jnext; // Next jump label.
                uc_function_attributes_t f; // Function attributes.
				int32_t auxtype; // Bit-field access type.
            };
        };

        int64_t enum_val; // Enum constant if IS_ENUM_VAL.
		int32_t* d; // Define token stream.
    };

    uc_c_type_t type; // Associated type.

    union 
	{
        struct uc_symbol_s* next; // Next related symbol (for fields and anoms).
		int32_t asm_label; // Associated assembler label.
    };

    struct uc_symbol_s* prev; // Previous symbol on the stack.
    struct uc_symbol_s* prev_tok; // Previous symbol for this token.
}
uc_symbol_t;


// Section definition.
typedef struct uc_section_s 
{
	uint32_t data_offset; // Current data offset.
	uint8_t* data; // Section data.
	uint32_t data_allocated; // Used for realloc handling.
	int32_t sh_name; // ELF section name (only used during output).
	int32_t sh_num; // ELF section number.
	int32_t sh_type; // ELF section type.
	int32_t sh_flags; // ELF section flags.
	int32_t sh_info; // ELF section info.
	int32_t sh_addralign; // ELF section alignment.
	int32_t sh_entsize; // ELF entry size.
	uint32_t sh_size; // Section size (only used during output).
	addr_t sh_addr; // Address at which the section is relocated.
	uint32_t sh_offset; // File offset.
	int32_t nb_hashed_syms; // Used to resize the hash table.
	struct uc_section_s *link; // Link to another section.
	struct uc_section_s *reloc; // Corresponding section for relocation, if any.
	struct uc_section_s *hash; // Hash table for symbols.
	struct uc_section_s *prev; // Previous section on section stack.
	char name[1]; // Section name.
}
uc_section_t;


typedef struct uc_dll_reference_s 
{
	int32_t level;
    void* handle;
    char name[1];
}
uc_dll_reference_t;


#define SYM_STRUCT     0x40000000 /* struct/union/enum symbol space */
#define SYM_FIELD      0x20000000 /* struct/union field symbol space */
#define SYM_FIRST_ANOM 0x10000000 /* first anonymous sym */


// Stored in 'uc_symbol_t->f.func_type' field.


#define FUNC_NEW       1 // ANSI function prototype.
#define FUNC_OLD       2 // Old function prototype.
#define FUNC_ELLIPSIS  3 // ANSI function prototype with '...'.


// Stored in 'uc_symbol_t->f.func_call' field.


#define FUNC_CDECL     0 // Standard C call.
#define FUNC_STDCALL   1 // Pascal C call.
#define FUNC_FASTCALL1 2 // First param in %eax.
#define FUNC_FASTCALL2 3 // First parameters in %eax, %edx.
#define FUNC_FASTCALL3 4 // First parameter in %eax, %edx, %ecx.
#define FUNC_FASTCALLW 5 // First parameter in %ecx, %edx.


// Field 'uc_symbol_t.t' for macros.


#define MACRO_OBJ      0 // Object like macro.
#define MACRO_FUNC     1 // Function like macro.


/* field 'uc_symbol_t.r' for C labels */
#define LABEL_DEFINED  0 /* label is defined */
#define LABEL_FORWARD  1 /* label is forward defined */
#define LABEL_DECLARED 2 /* label is declared but never used */


/* type_decl() types */
#define TYPE_ABSTRACT  1 /* type without variable */
#define TYPE_DIRECT    2 /* type with variable */


typedef struct uc_buffered_file_s 
{
    uint8_t *buf_ptr;
    uint8_t *buf_end;
	int32_t fd;
    struct uc_buffered_file_s* prev;
	int32_t line_num;    /* current line number - here to simplify code */
	int32_t line_ref;    /* tcc -E: last printed line */
	int32_t ifndef_macro;  /* #ifndef macro / #endif search */
	int32_t ifndef_macro_saved; /* saved ifndef_macro */
	int32_t* ifdef_stack_ptr; /* ifdef_stack value at the start of the file */
	int32_t include_next_index; /* next search path */
    char filename[1024];    /* filename */
    char* true_filename; /* filename not modified by #line directive */
    uint8_t unget[4];
	uint8_t buffer[1]; /* extra size for CH_EOB char */
}
uc_buffered_file_t;


#define CH_EOB   '\\'       /* end of buffer or '\0' char in file */
#define CH_EOF   (-1)   /* end of file */


// Used to record tokens.
typedef struct uc_token_string_s 
{
    int32_t *str;
	int32_t len;
	int32_t lastlen;
	int32_t allocated_len;
	int32_t last_line_num;
	int32_t save_line_num;

    // Used to chain token-strings with begin / end macro.
    struct uc_token_string_s* prev;
    const int32_t* prev_ptr;
    char alloc;
}
uc_token_string_t;


// Inline function.
typedef struct uc_inline_function_s 
{
    uc_token_string_t* func_str;
    uc_symbol_t* sym;
    char filename[1];
}
uc_inline_function_t;


// Include file cache, used to find files faster and also to eliminate inclusion 
// if the include file is protected by #ifndef/#endif.
typedef struct uc_cached_include_s
{
	int32_t ifndef_macro;
	int32_t once;
	int32_t hash_next; // -1 if none.
    char filename[1]; // Path specified in #include.
}
uc_cached_include_t;


typedef struct uc_expression_value_s 
{
    uint64_t v;
    uc_symbol_t* sym;
	int32_t pcrel;
} 
uc_expression_value_t;


typedef struct uc_assembler_operand_s 
{
	int32_t id; // GCC 3 optional identifier (0 if number only supported).
    char* constraint;
    char asm_str[16]; // Computed assembly string for operand.
    uc_stack_value_t* vt; // C value of the expression.
	int32_t ref_index; // If >= 0, gives reference to a output constraint.
	int32_t input_index; // If >= 0, gives reference to an input constraint.
	int32_t priority; // Priority, used to assign registers.
	int32_t reg; // If >= 0, register number used for this operand.
	bool is_llong; // True if double register value.
	bool is_memory; // True if memory operand.
	bool is_rw;     // For '+' modifier.
} 
uc_assembler_operand_t;


// Extra symbol attributes (not in symbol table).
typedef struct uc_symbol_attributes_x_s 
{
    uint32_t got_offset;
	uint32_t plt_offset;
	int32_t plt_sym;
	int32_t dyn_index;
    uint8_t plt_thumb_stub : 1; // For ARM.
}
uc_symbol_attributes_x_t;


// Compiler state.
typedef struct uc_state_s
{
	// If true, display verbose information during compilation.
	bool verbose;

	// If true, no standard headers are added.
	bool nostdinc;

	// If true, no standard libraries are added.
	bool nostdlib;

	// If true, do not use common symbols for '.bss' data.
	bool nocommon;

	// If true, static linking is performed.
	bool static_link;

	// If true, all symbols are exported.
	bool rdynamic;

	// If true, resolve symbols in the current module first.
	bool symbolic;

	// If true, only link in referenced objects from archive.
	bool alacarte_link;

	// CONFIG_TCCDIR or '-B' option.
	char* tcc_lib_path;

	// As specified on the command line ('-soname').
	char* soname;

	// As specified on the command line ('-Wl', '-rpath=..').
	char* rpath;

	// As specified on the command line ('-Wl', '--enable-new-dtags').
	bool enable_new_dtags;

	// Output type, see 'TCC_OUTPUT_*.'
	int32_t output_type;

	// Output format, see 'TCC_OUTPUT_FORMAT_*.'
	int32_t output_format;

	// C language options.

	bool char_is_unsigned;
	bool leading_underscore;

	// Allow nested named struct without identifier behave like unnamed.
	bool ms_extensions;

	// If true, allows '$' in identifiers.
	int dollars_in_identifiers;

	// If true, emulate MS algorithm for aligning bit-fields.
	bool ms_bitfields;

	// Warning switches.

	bool warn_write_strings;
	bool warn_unsupported;
	bool warn_error;
	bool warn_none;
	bool warn_implicit_function_declaration;
	bool warn_gcc_compat;

	// Compile with debug symbol (and use them if error during execution).
	bool do_debug;

	// Compile with built-in memory and bounds checker.
	bool do_bounds_check;

	// ARM float ABI of the generated code.
	enum float_abi float_abi;

	// N-th test to run with '-dt' and '-run'.
	int run_test;

	// Address of text section.
	addr_t text_addr;
	bool has_text_addr;

	// Section alignment.
	uint32_t section_align;

	// Symbols to call at load-time (not currently used).
	char* init_symbol;

	// Symbols to call at unload-time (not currently used).
	char* fini_symbol;

	// For i386, defaults to 32, but can be 16 with i386 assembler ('.code16').
	int32_t seg_size;

	// For x86-64 '-mno-sse' support.
	bool nosse;

	// Array of all loaded DLLs (including those referenced by loaded DLLs).
	uc_dll_reference_t** loaded_dlls;
	int32_t nb_loaded_dlls;

	// Include paths.
	char** include_paths;
	int32_t nb_include_paths;

	char** sysinclude_paths;
	int32_t nb_sysinclude_paths;

	// Library paths.
	char** library_paths;
	int32_t nb_library_paths;

	// The crt*.a object path.
	char** crt_paths;
	int32_t nb_crt_paths;

	// Include files.
	char** cmd_include_files;
	int32_t nb_cmd_include_files;

	// Error handling.
	void* error_opaque;
	void (*error_func)(void* opaque, const char* msg);
	bool error_set_jmp_enabled;
	jmp_buf error_jmp_buf;
	int32_t nb_errors;

	// Output file for preprocessing ('-E').
	FILE* ppfp;

	// For the '-P' switch.
	enum
	{
		LINE_MACRO_OUTPUT_FORMAT_GCC,
		LINE_MACRO_OUTPUT_FORMAT_NONE,
		LINE_MACRO_OUTPUT_FORMAT_STD,
		LINE_MACRO_OUTPUT_FORMAT_P10 = 11
	}
	Pflag;

	// For the '-dX' value.
	char dflag;

	// For '-MD' and '-MF', collected dependencies for this compilation.
	char** target_deps;
	int32_t nb_target_deps;

	// Compilation.
	uc_buffered_file_t* include_stack[INCLUDE_STACK_SIZE];
	uc_buffered_file_t** include_stack_ptr;

	int32_t ifdef_stack[IFDEF_STACK_SIZE];
	int32_t* ifdef_stack_ptr;

	// Included files enclosed with #ifndef macro.
	int32_t cached_includes_hash[CACHED_INCLUDES_HASH_SIZE];

	uc_cached_include_t** cached_includes;
	int32_t nb_cached_includes;

	// The #pragma pack stack.
	int32_t pack_stack[PACK_STACK_SIZE];
	int32_t* pack_stack_ptr;
	char** pragma_libs;
	int32_t nb_pragma_libs;

	// Inline functions are stored as token lists and compiled last only if referenced.
	struct uc_inline_function_s** inline_fns;
	int32_t nb_inline_fns;

	// Sections.
	uc_section_t** sections;

	// Number of sections, including first dummy section.
	int32_t nb_sections;

	uc_section_t** priv_sections;

	// Number of private sections.
	int nb_priv_sections;

	// GOT and PLT handling.
	uc_section_t* got;
	uc_section_t* plt;

	// Temporary dynamic symbol sections (for DLL loading).
	uc_section_t* dynsymtab_section;

	// Exported dynamic symbol section.
	uc_section_t* dynsym;

	// Copy of the global symtab_section variable.
	uc_section_t* symtab;

	// Extra attributes (eg. GOT / PLT value) for symbol table symbols.
	struct uc_symbol_attributes_x_s* sym_attrs;
	int32_t nb_sym_attrs;

    // For PE.
	int32_t pe_subsystem;
    uint32_t pe_characteristics;
    uint32_t pe_file_align;
    uint32_t pe_stack_size;
    addr_t pe_imagebase;

	// For PE x86-64.
    uc_section_t* uw_pdata;
	int32_t uw_sym;
    uint32_t uw_offs;

	// Native mode.
    const char* runtime_main;
    void** runtime_mem;
    uint32_t nb_runtime_mem;

    // Used by main and tcc_parse_args only.

    struct uc_file_spec_s** files; // Files seen on command line.
	int32_t nb_files; // Number of files.
	int32_t nb_libraries; // Number of libraries.
	int32_t filetype;

    char* outfile; // Output file name.
    bool option_r; // Option '-r'.
	bool do_bench; // Option '-bench'.
	bool gen_deps; // Option '-MD'.
    char* deps_outfile; // Option '-MF'.
	bool option_pthread; // The '-pthread' option.

    int argc;
    char** argv;
}
uc_state_t;


typedef struct uc_file_spec_s 
{
    char type;
    char alacarte;
    char name[1];
}
uc_file_spec_t;


// The current value can be:


#define VT_VALMASK			0x0000003F // Mask for value location, register or the following.
#define VT_CONST			0x00000030 // Constant in VC (must be first non-register value).
#define VT_LLOCAL			0x00000031 // L-value, offset on stack.
#define VT_LOCAL			0x00000032 // Offset on stack.
#define VT_CMP				0x00000033 // The value is stored in processor flags (in VC).
#define VT_JMP				0x00000034 // Value is the consequence of JMP true (even).
#define VT_JMPI				0x00000035 // Value is the consequence of JMP false (odd).
#define VT_LVAL				0x00000100 // Variable is an L-value.
#define VT_SYM				0x00000200 // A symbol value is added.
#define VT_MUSTCAST			0x00000400 // Value must be cast to be correct (used for char / short stored in integer registers).
#define VT_MUSTBOUND		0x00000800 // Bound checking must be done before dereferencing value.
#define VT_BOUNDED			0x00008000 // Value is bounded. The address of the bounding function call point is in VC.
#define VT_LVAL_BYTE		0x00001000 // L-value is a byte.
#define VT_LVAL_SHORT		0x00002000 // L-value is a short.
#define VT_LVAL_UNSIGNED	0x00004000 // L-value is unsigned.
#define VT_LVAL_TYPE		(VT_LVAL_BYTE | VT_LVAL_SHORT | VT_LVAL_UNSIGNED)


// Types.


#define VT_BTYPE			0x0000000F // Mask for basic type.
#define VT_VOID				0x00000000 // Void type.
#define VT_BYTE				0x00000001 // Signed byte type.
#define VT_SHORT			0x00000002 // Short type.
#define VT_INT				0x00000003 // Integer type.
#define VT_LLONG			0x00000004 // 64 bit integer.
#define VT_PTR				0x00000005 // Pointer.
#define VT_FUNC				0x00000006 // Function type.
#define VT_STRUCT			0x00000007 // Struct / union definition.
#define VT_FLOAT			0x00000008 // IEEE float.
#define VT_DOUBLE			0x00000009 // IEEE double.
#define VT_LDOUBLE			0x0000000A // IEEE long double.
#define VT_BOOL				0x0000000B // ISO C99 boolean type.
#define VT_QLONG			0x0000000D // 128-bit integer. Only used for x86-64 ABI.
#define VT_QFLOAT			0x0000000E // 128-bit float. Only used for x86-64 ABI.


#define VT_UNSIGNED			0x00000010 // Unsigned type.
#define VT_DEFSIGN			0x00000020 // Explicitly signed or unsigned.
#define VT_ARRAY			0x00000040 // Array type (also has VT_PTR).
#define VT_BITFIELD			0x00000080 // Bit-field modifier.
#define VT_CONSTANT			0x00000100 // Constant modifier.
#define VT_VOLATILE			0x00000200 // Volatile modifier.
#define VT_VLA				0x00000400 // VLA type (also has VT_PTR and VT_ARRAY).
#define VT_LONG				0x00000800 // Long type (also has VT_INT rsp. VT_LLONG).


// Storage.


#define VT_EXTERN			0x00001000 // Extern definition.
#define VT_STATIC			0x00002000 // Static variable.
#define VT_TYPEDEF			0x00004000 // Typedef definition.
#define VT_INLINE			0x00008000 // Inline definition.


#define VT_STRUCT_SHIFT		0x00000014 // Shift for bit-field shift values (32 - 2 * 6).
#define VT_STRUCT_MASK		(((0x01 << 0x0C) - 0x01) << VT_STRUCT_SHIFT | VT_BITFIELD)
#define BIT_POS(V)			(((V) >> VT_STRUCT_SHIFT) & 0x3F)
#define BIT_SIZE(V)			(((V) >> (VT_STRUCT_SHIFT + 0x06)) & 0x3F)


#define VT_UNION			(0x01 << VT_STRUCT_SHIFT | VT_STRUCT)
#define VT_ENUM				(0x02 << VT_STRUCT_SHIFT) // Integral type is actually an enumeration.
#define VT_ENUM_VAL			(0x03 << VT_STRUCT_SHIFT) // Integral type is actually an enum constant.


#define IS_ENUM(V)			((V & VT_STRUCT_MASK) == VT_ENUM)
#define IS_ENUM_VAL(V)		((V & VT_STRUCT_MASK) == VT_ENUM_VAL)
#define IS_UNION(V)			((V & (VT_STRUCT_MASK | VT_BTYPE)) == VT_UNION)


// Type mask (except storage).


#define VT_STORAGE			(VT_EXTERN | VT_STATIC | VT_TYPEDEF | VT_INLINE)
#define VT_TYPE				(~(VT_STORAGE|VT_STRUCT_MASK))


// Symbol was created by asm.c first.


#define VT_ASM				(VT_VOID | VT_UNSIGNED)
#define IS_ASM_SYM(S)		(((S)->type.t & (VT_BTYPE | VT_ASM)) == VT_ASM)


// Token values.


// Warning: The following compare tokens depend on x86 assembly.


#define TOK_ULT				0x92
#define TOK_UGE				0x93
#define TOK_EQ				0x94
#define TOK_NE				0x95
#define TOK_ULE				0x96
#define TOK_UGT				0x97
#define TOK_Nset			0x98
#define TOK_Nclear			0x99
#define TOK_LT				0x9C
#define TOK_GE				0x9D
#define TOK_LE				0x9E
#define TOK_GT				0x9F
#define TOK_LAND			0xA0
#define TOK_LOR				0xA1
#define TOK_DEC				0xA2
#define TOK_MID				0xA3 // Increment / decrement, to void constant.
#define TOK_INC				0xA4
#define TOK_UDIV			0xB0 // Unsigned division.
#define TOK_UMOD			0xB1 // Unsigned modulus.
#define TOK_PDIV			0xB2 // Fast division, with undefined rounding for pointers.


// Tokens that carry values (in additional token string space / 'tokc').


#define TOK_CCHAR			0xB3 // Character constant in tokc.
#define TOK_LCHAR			0xB4
#define TOK_CINT			0xB5 // Number in 'tokc'.
#define TOK_CUINT			0xB6 // Unsigned integer constant.
#define TOK_CLLONG			0xB7 // Long long constant.
#define TOK_CULLONG			0xB8 // Unsigned long long constant.
#define TOK_STR				0xB9 // Pointer to string in 'tokc'.
#define TOK_LSTR			0xBA
#define TOK_CFLOAT			0xBB // Float constant.
#define TOK_CDOUBLE			0xBC // Double constant.
#define TOK_CLDOUBLE		0xBD // Long double constant.
#define TOK_PPNUM			0xBE // Preprocessor number.
#define TOK_PPSTR			0xBF // Preprocessor string.
#define TOK_LINENUM			0xC0 // Line number information.
#define TOK_TWODOTS			0xA8 // C++ token?


#define TOK_UMULL			0xC2 // Unsigned 32 x 32 -> 64 multiplication.
#define TOK_ADDC1			0xC3 // Add with carry generation.
#define TOK_ADDC2			0xC4 // Add with carry use.
#define TOK_SUBC1			0xC5 // Add with carry generation.
#define TOK_SUBC2			0xC6 // Add with carry use.
#define TOK_ARROW			0xC7 // '->'
#define TOK_DOTS			0xC8 // Ellipsis.
#define TOK_SHR				0xC9 // Unsigned shift right.
#define TOK_TWOSHARPS		0xCA // Preprocessing token.
#define TOK_PLCHLDR			0xCB // Placeholder token as defined in C99.
#define TOK_NOSUBST			0xCC // Indicates that the following token has already been preprocessed.
#define TOK_PPJOIN			0xCD // A '##' in the right position to mean pasting.
#define TOK_CLONG			0xCE // Long constant.
#define TOK_CULONG			0xCF // Unsigned long constant.


#define TOK_SHL				0x01 // Shift left.
#define TOK_SAR				0x02 // Signed shift right.


// Assignment operators: Normal operator or 0x80.


#define TOK_A_MOD			0xA5
#define TOK_A_AND			0xA6
#define TOK_A_MUL			0xAA
#define TOK_A_ADD			0xAB
#define TOK_A_SUB			0xAD
#define TOK_A_DIV			0xAF
#define TOK_A_XOR			0xDE
#define TOK_A_OR			0xFC
#define TOK_A_SHL			0x81
#define TOK_A_SAR			0x82


#define TOK_EOF				(-1) // End of file.
#define TOK_LINEFEED		0x0A // Line feed.

// All identifiers and strings have token above that.
#define TOK_IDENT			0x100


#define DEF_ASM(x) DEF(TOK_ASM_ ##x, #x)
#define TOK_ASM_int TOK_INT
#define DEF_ASMDIR(x) DEF(TOK_ASMDIR_ ##x, "." #x)
#define TOK_ASMDIR_FIRST TOK_ASMDIR_byte
#define TOK_ASMDIR_LAST TOK_ASMDIR_section


#if defined(TCC_TARGET_I386) || defined(TCC_TARGET_X86_64)


// Only used for x86 assembler opcode definitions.


#define DEF_BWL(x) DEF(TOK_ASM_ ##x ##b, #x "b") DEF(TOK_ASM_ ##x ##w, #x "w") \
	DEF(TOK_ASM_ ##x ##l, #x "l") DEF(TOK_ASM_ ##x, #x)


#define DEF_WL(x) DEF(TOK_ASM_ ##x ##w, #x "w") DEF(TOK_ASM_ ##x ##l, #x "l") DEF(TOK_ASM_ ##x, #x)


#ifdef TCC_TARGET_X86_64


#define DEF_BWLQ(x) DEF(TOK_ASM_ ##x ##b, #x "b") DEF(TOK_ASM_ ##x ##w, #x "w") \
	DEF(TOK_ASM_ ##x ##l, #x "l") DEF(TOK_ASM_ ##x ##q, #x "q") DEF(TOK_ASM_ ##x, #x)


#define DEF_WLQ(x) DEF(TOK_ASM_ ##x ##w, #x "w") DEF(TOK_ASM_ ##x ##l, #x "l") \
	DEF(TOK_ASM_ ##x ##q, #x "q") DEF(TOK_ASM_ ##x, #x)


#define DEF_BWLX DEF_BWLQ
#define DEF_WLX DEF_WLQ


#define NBWLX 5 // Number of sizes + 1.


#else // !TCC_TARGET_X86_64


#define DEF_BWLX DEF_BWL
#define DEF_WLX DEF_WL


#define NBWLX 4 // Number of sizes + 1.


#endif // TCC_TARGET_X86_64


#define DEF_FP1(x) DEF(TOK_ASM_ ##f ##x ##s, "f" #x "s") DEF(TOK_ASM_ ##fi ##x ##l, "fi" #x "l") \
	DEF(TOK_ASM_ ##f ##x ##l, "f" #x "l") DEF(TOK_ASM_ ##fi ##x ##s, "fi" #x "s")

#define DEF_FP(x) DEF(TOK_ASM_ ##f ##x, "f" #x ) DEF(TOK_ASM_ ##f ##x ##p, "f" #x "p") DEF_FP1(x)


#define DEF_ASMTEST(x,suffix) \
DEF_ASM(x ##o ##suffix) \
DEF_ASM(x ##no ##suffix) \
DEF_ASM(x ##b ##suffix) \
DEF_ASM(x ##c ##suffix) \
DEF_ASM(x ##nae ##suffix) \
DEF_ASM(x ##nb ##suffix) \
DEF_ASM(x ##nc ##suffix) \
DEF_ASM(x ##ae ##suffix) \
DEF_ASM(x ##e ##suffix) \
DEF_ASM(x ##z ##suffix) \
DEF_ASM(x ##ne ##suffix) \
DEF_ASM(x ##nz ##suffix) \
DEF_ASM(x ##be ##suffix) \
DEF_ASM(x ##na ##suffix) \
DEF_ASM(x ##nbe ##suffix) \
DEF_ASM(x ##a ##suffix) \
DEF_ASM(x ##s ##suffix) \
DEF_ASM(x ##ns ##suffix) \
DEF_ASM(x ##p ##suffix) \
DEF_ASM(x ##pe ##suffix) \
DEF_ASM(x ##np ##suffix) \
DEF_ASM(x ##po ##suffix) \
DEF_ASM(x ##l ##suffix) \
DEF_ASM(x ##nge ##suffix) \
DEF_ASM(x ##nl ##suffix) \
DEF_ASM(x ##ge ##suffix) \
DEF_ASM(x ##le ##suffix) \
DEF_ASM(x ##ng ##suffix) \
DEF_ASM(x ##nle ##suffix) \
DEF_ASM(x ##g ##suffix)


#endif // defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64


enum uc_token_e
{
	TOK_LAST = TOK_IDENT - 1

#define DEF(id, str) , id

#include "tok.h"

#undef DEF
};


#define TOK_UIDENT TOK_DEFINE


extern int gnu_ext;
extern int tcc_ext;
extern struct uc_state_s* tcc_state;


ST_FUNC char *pstrcpy(char *buf, int buf_size, const char *s);
ST_FUNC char *pstrcat(char *buf, int buf_size, const char *s);
ST_FUNC char *pstrncpy(char *out, const char *in, size_t num);
PUB_FUNC char *tcc_basename(const char *name);
PUB_FUNC char *tcc_fileextension (const char *name);


#ifndef MEM_DEBUG

PUB_FUNC void tcc_free(void *ptr);
PUB_FUNC void *tcc_malloc(unsigned long size);
PUB_FUNC void *tcc_mallocz(unsigned long size);
PUB_FUNC void *tcc_realloc(void *ptr, unsigned long size);
PUB_FUNC char *tcc_strdup(const char *str);

#else

#define tcc_free(ptr)           tcc_free_debug(ptr)
#define tcc_malloc(size)        tcc_malloc_debug(size, __FILE__, __LINE__)
#define tcc_mallocz(size)       tcc_mallocz_debug(size, __FILE__, __LINE__)
#define tcc_realloc(ptr,size)   tcc_realloc_debug(ptr, size, __FILE__, __LINE__)
#define tcc_strdup(str)         tcc_strdup_debug(str, __FILE__, __LINE__)

PUB_FUNC void tcc_free_debug(void *ptr);
PUB_FUNC void *tcc_malloc_debug(unsigned long size, const char *file, int line);
PUB_FUNC void *tcc_mallocz_debug(unsigned long size, const char *file, int line);
PUB_FUNC void *tcc_realloc_debug(void *ptr, unsigned long size, const char *file, int line);
PUB_FUNC char *tcc_strdup_debug(const char *str, const char *file, int line);

#endif

#define free(p) use_tcc_free(p)
#define malloc(s) use_tcc_malloc(s)
#define realloc(p, s) use_tcc_realloc(p, s)


#undef strdup
#define strdup(s) use_tcc_strdup(s)


PUB_FUNC void tcc_memcheck(void);
PUB_FUNC void tcc_error_noabort(const char *fmt, ...);
PUB_FUNC NORETURN void tcc_error(const char *fmt, ...);
PUB_FUNC void tcc_warning(const char *fmt, ...);


ST_FUNC void dynarray_add(void *ptab, int *nb_ptr, void *data);
ST_FUNC void dynarray_reset(void *pp, int *n);
ST_INLN void cstr_ccat(uc_string_t *cstr, int ch);
ST_FUNC void cstr_cat(uc_string_t *cstr, const char *str, int len);
ST_FUNC void cstr_wccat(uc_string_t *cstr, int ch);
ST_FUNC void cstr_new(uc_string_t *cstr);
ST_FUNC void cstr_free(uc_string_t *cstr);
ST_FUNC void cstr_reset(uc_string_t *cstr);

ST_INLN void sym_free(uc_symbol_t *sym);
ST_FUNC uc_symbol_t *sym_push2(uc_symbol_t **ps, int v, int t, int c);
ST_FUNC uc_symbol_t *sym_find2(uc_symbol_t *s, int v);
ST_FUNC uc_symbol_t *sym_push(int v, uc_c_type_t *type, int r, int c);
ST_FUNC void sym_pop(uc_symbol_t **ptop, uc_symbol_t *b, int keep);
ST_INLN uc_symbol_t *struct_find(int v);
ST_INLN uc_symbol_t *sym_find(int v);
ST_FUNC uc_symbol_t *global_identifier_push(int v, int t, int c);

ST_FUNC void tcc_open_bf(uc_state_t *s1, const char *filename, int initlen);
ST_FUNC int tcc_open(uc_state_t *s1, const char *filename);
ST_FUNC void tcc_close(void);

ST_FUNC int tcc_add_file_internal(uc_state_t *s1, const char *filename, int flags);

/* flags: */
#define AFF_PRINT_ERROR     0x10 /* print error if file not found */
#define AFF_REFERENCED_DLL  0x20 /* load a referenced dll from another dll */
#define AFF_TYPE_BIN        0x40 /* file to add is binary */

/* s->filetype: */
#define AFF_TYPE_NONE   0
#define AFF_TYPE_C      1
#define AFF_TYPE_ASM    2
#define AFF_TYPE_ASMPP  3
#define AFF_TYPE_LIB    4

/* values from tcc_object_type(...) */
#define AFF_BINTYPE_REL 1
#define AFF_BINTYPE_DYN 2
#define AFF_BINTYPE_AR  3
#define AFF_BINTYPE_C67 4


ST_FUNC int tcc_add_crt(uc_state_t *s, const char *filename);
ST_FUNC int tcc_add_dll(uc_state_t *s, const char *filename, int flags);
ST_FUNC void tcc_add_pragma_libs(uc_state_t *s1);
PUB_FUNC int tcc_add_library_err(uc_state_t *s, const char *f);
PUB_FUNC void tcc_print_stats(uc_state_t *s, unsigned total_time);
PUB_FUNC int tcc_parse_args(uc_state_t *s, int *argc, char ***argv, int optind);
#ifdef _WIN32
ST_FUNC char *normalize_slashes(char *path);
#endif

/* tcc_parse_args return codes: */
#define OPT_HELP 1
#define OPT_HELP2 2
#define OPT_V 3
#define OPT_PRINT_DIRS 4
#define OPT_AR 5
#define OPT_IMPDEF 6
#define OPT_M32 32
#define OPT_M64 64



extern struct uc_buffered_file_s* file;
extern int ch, tok;
extern uc_const_value_t tokc;
extern const int *macro_ptr;
extern int parse_flags;
extern int tok_flags;
extern uc_string_t tokcstr; /* current parsed string, if any */

/* display benchmark infos */

extern int total_lines;
extern int total_bytes;
extern int tok_ident;
extern uc_token_symbol_t** table_ident;

#define TOK_FLAG_BOL   0x0001 /* beginning of line before */
#define TOK_FLAG_BOF   0x0002 /* beginning of file before */
#define TOK_FLAG_ENDIF 0x0004 /* a endif was found matching starting #ifdef */
#define TOK_FLAG_EOF   0x0008 /* end of file */

#define PARSE_FLAG_PREPROCESS 0x0001 /* activate preprocessing */
#define PARSE_FLAG_TOK_NUM    0x0002 /* return numbers instead of TOK_PPNUM */
#define PARSE_FLAG_LINEFEED   0x0004 /* line feed is returned as a
                                        token. line feed is also
                                        returned at eof */
#define PARSE_FLAG_ASM_FILE 0x0008 /* we processing an asm file: '#' can be used for line comment, etc. */
#define PARSE_FLAG_SPACES     0x0010 /* next() returns space tokens (for -E) */
#define PARSE_FLAG_ACCEPT_STRAYS 0x0020 /* next() returns '\\' token */
#define PARSE_FLAG_TOK_STR    0x0040 /* return parsed strings instead of TOK_PPSTR */

/* isidnum_table flags: */
#define IS_SPC 1
#define IS_ID  2
#define IS_NUM 4

ST_FUNC uc_token_symbol_t *tok_alloc(const char *str, int len);
ST_FUNC const char *get_tok_str(int v, uc_const_value_t *cv);
ST_FUNC void begin_macro(uc_token_string_t *str, int alloc);
ST_FUNC void end_macro(void);
ST_FUNC int set_idnum(int c, int val);
ST_INLN void tok_str_new(uc_token_string_t *s);
ST_FUNC uc_token_string_t *tok_str_alloc(void);
ST_FUNC void tok_str_free(uc_token_string_t *s);
ST_FUNC void tok_str_free_str(int *str);
ST_FUNC void tok_str_add(uc_token_string_t *s, int t);
ST_FUNC void tok_str_add_tok(uc_token_string_t *s);
ST_INLN void define_push(int v, int macro_type, int *str, uc_symbol_t *first_arg);
ST_FUNC void define_undef(uc_symbol_t *s);
ST_INLN uc_symbol_t *define_find(int v);
ST_FUNC void free_defines(uc_symbol_t *b);
ST_FUNC uc_symbol_t *label_find(int v);
ST_FUNC uc_symbol_t *label_push(uc_symbol_t **ptop, int v, int flags);
ST_FUNC void label_pop(uc_symbol_t **ptop, uc_symbol_t *slast, int keep);
ST_FUNC void parse_define(void);
ST_FUNC void preprocess(int is_bof);
ST_FUNC void next_nomacro(void);
ST_FUNC void next(void);
ST_INLN void unget_tok(int last_tok);
ST_FUNC void preprocess_start(uc_state_t *s1, int is_asm);
ST_FUNC void preprocess_end(uc_state_t *s1);
ST_FUNC void tccpp_new(uc_state_t *s);
ST_FUNC void tccpp_delete(uc_state_t *s);
ST_FUNC int tcc_preprocess(uc_state_t *s1);
ST_FUNC void skip(int c);
ST_FUNC NORETURN void expect(const char *msg);


inline static bool is_space(int ch) { return ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f' || ch == '\r'; } // Excluding newline.
inline static bool isid(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
inline static bool isnum(int c) { return c >= '0' && c <= '9'; }
inline static bool isoct(int c) { return c >= '0' && c <= '7'; }
inline static int toup(int c) { return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c; }


// gen.c


#define SYM_POOL_NB (8192 / sizeof(uc_symbol_t))


ST_DATA uc_symbol_t *sym_free_first;
ST_DATA void **sym_pools;
ST_DATA int nb_sym_pools;

ST_DATA uc_symbol_t* global_stack;
ST_DATA uc_symbol_t* local_stack;
ST_DATA uc_symbol_t* local_label_stack;
ST_DATA uc_symbol_t* global_label_stack;
ST_DATA uc_symbol_t* define_stack;
ST_DATA uc_c_type_t char_pointer_type, func_old_type, int_type, size_type;
ST_DATA uc_stack_value_t __vstack[1 + /* for bcheck */ VSTACK_SIZE], *vtop, *pvtop;

#define vstack  (__vstack + 1)

ST_DATA int rsym, anon_sym, ind, loc;


ST_DATA int const_wanted; /* true if constant wanted */
ST_DATA int nocode_wanted; /* true if no code generation wanted for an expression */
ST_DATA int global_expr;  /* true if compound literals must be allocated globally (used during initializers parsing */
ST_DATA uc_c_type_t func_vt; /* current function return type (used by return instruction) */
ST_DATA int func_var; /* true if current function is variadic */
ST_DATA int func_vc;
ST_DATA int last_line_num, last_ind, func_ind; /* debug last line number and pc */
ST_DATA const char *funcname;
ST_DATA int g_debug;


ST_FUNC void tcc_debug_start(uc_state_t *s1);
ST_FUNC void tcc_debug_end(uc_state_t *s1);
ST_FUNC void tcc_debug_funcstart(uc_state_t *s1, uc_symbol_t *sym);
ST_FUNC void tcc_debug_funcend(uc_state_t *s1, int size);
ST_FUNC void tcc_debug_line(uc_state_t *s1);


ST_FUNC int tccgen_compile(uc_state_t *s1);
ST_FUNC void free_inline_functions(uc_state_t *s);
ST_FUNC void check_vstack(void);


ST_INLN int is_float(int t);
ST_FUNC int ieee_finite(double d);
ST_FUNC void test_lvalue(void);
ST_FUNC void vpushi(int v);
ST_FUNC ElfSym* elfsym(uc_symbol_t *);
ST_FUNC void update_storage(uc_symbol_t *sym);
ST_FUNC uc_symbol_t* external_global_sym(int v, uc_c_type_t *type, int r);
ST_FUNC void vset(uc_c_type_t *type, int r, int v);
ST_FUNC void vswap(void);
ST_FUNC void vpush_global_sym(uc_c_type_t *type, int v);
ST_FUNC void vrote(uc_stack_value_t *e, int n);
ST_FUNC void vrott(int n);
ST_FUNC void vrotb(int n);


// ARM.
ST_FUNC int get_reg_ex(int rc, int rc2);
ST_FUNC void lexpand_nr(void);


ST_FUNC void vpushv(uc_stack_value_t *v);
ST_FUNC void save_reg(int r);
ST_FUNC void save_reg_upstack(int r, int n);
ST_FUNC int get_reg(int rc);
ST_FUNC void save_regs(int n);
ST_FUNC void gaddrof(void);
ST_FUNC int gv(int rc);
ST_FUNC void gv2(int rc1, int rc2);
ST_FUNC void vpop(void);
ST_FUNC void gen_op(int op);
ST_FUNC int type_size(uc_c_type_t *type, int *a);
ST_FUNC void mk_pointer(uc_c_type_t *type);
ST_FUNC void vstore(void);
ST_FUNC void inc(int post, int c);
ST_FUNC void parse_mult_str (uc_string_t *astr, const char *msg);
ST_FUNC void parse_asm_str(uc_string_t *astr);
ST_FUNC int lvalue_type(int t);
ST_FUNC void indir(void);
ST_FUNC void unary(void);
ST_FUNC void expr_prod(void);
ST_FUNC void expr_sum(void);
ST_FUNC void gexpr(void);
ST_FUNC int expr_const(void);


#if defined CONFIG_TCC_BCHECK
ST_FUNC uc_symbol_t *get_sym_ref(uc_c_type_t *type, uc_section_t *sec, unsigned long offset, unsigned long size);
#endif


#if defined TCC_TARGET_X86_64 && !defined TCC_TARGET_PE
ST_FUNC int classify_x86_64_va_arg(uc_c_type_t *ty);
#endif


// elf.c


#define TCC_OUTPUT_FORMAT_ELF    0 /* default output format: ELF */
#define TCC_OUTPUT_FORMAT_BINARY 1 /* binary image output */
#define TCC_OUTPUT_FORMAT_COFF   2 /* COFF */


#define ARMAG  "!<arch>\012"    /* For COFF and a.out archives */


typedef struct uc_dbx_symbol_s
{
    unsigned int n_strx;         /* index into string table of name */
    unsigned char n_type;         /* type of symbol */
    unsigned char n_other;        /* misc info (usually empty) */
    unsigned short n_desc;        /* description field */
    unsigned int n_value;        /* value of symbol */
}
uc_dbx_symbol_t;


ST_DATA uc_section_t* text_section, *data_section, *bss_section; /* predefined sections */
ST_DATA uc_section_t* common_section;
ST_DATA uc_section_t* cur_text_section; /* current section where function code is generated */


#ifdef CONFIG_TCC_ASM
ST_DATA uc_section_t* last_text_section; /* to handle .previous asm directive */
#endif


// Bound check related sections.


#ifdef CONFIG_TCC_BCHECK
ST_DATA uc_section_t* bounds_section; /* contains global data bound description */
ST_DATA uc_section_t* lbounds_section; /* contains local data bound description */
ST_FUNC void tccelf_bounds_new(uc_state_t *s);
#endif


// Symbol sections.


ST_DATA uc_section_t* symtab_section;


// Debug sections.


ST_DATA uc_section_t* stab_section, *stabstr_section;


ST_FUNC void tccelf_new(uc_state_t *s);
ST_FUNC void tccelf_delete(uc_state_t *s);
ST_FUNC void tccelf_stab_new(uc_state_t *s);
ST_FUNC void tccelf_begin_file(uc_state_t *s1);
ST_FUNC void tccelf_end_file(uc_state_t *s1);


ST_FUNC uc_section_t* new_section(uc_state_t *s1, const char *name, int sh_type, int sh_flags);
ST_FUNC void section_realloc(uc_section_t* sec, unsigned long new_size);
ST_FUNC size_t section_add(uc_section_t* sec, addr_t size, int align);
ST_FUNC void *section_ptr_add(uc_section_t* sec, addr_t size);
ST_FUNC void section_reserve(uc_section_t* sec, unsigned long size);
ST_FUNC uc_section_t* find_section(uc_state_t *s1, const char *name);
ST_FUNC uc_section_t* new_symtab(uc_state_t *s1, const char *symtab_name, int sh_type, int sh_flags, const char *strtab_name, const char *hash_name, int hash_sh_flags);


ST_FUNC void put_extern_sym2(uc_symbol_t *sym, int sh_num, addr_t value, unsigned long size, int can_add_underscore);
ST_FUNC void put_extern_sym(uc_symbol_t *sym, uc_section_t* section, addr_t value, unsigned long size);


#if PTR_SIZE == 4
ST_FUNC void greloc(uc_section_t* s, uc_symbol_t *sym, unsigned long offset, int type);
#endif


ST_FUNC void greloca(uc_section_t* s, uc_symbol_t *sym, unsigned long offset, int type, addr_t addend);


ST_FUNC int put_elf_str(uc_section_t* s, const char *sym);
ST_FUNC int put_elf_sym(uc_section_t* s, addr_t value, unsigned long size, int info, int other, int shndx, const char *name);
ST_FUNC int set_elf_sym(uc_section_t* s, addr_t value, unsigned long size, int info, int other, int shndx, const char *name);
ST_FUNC int find_elf_sym(uc_section_t* s, const char *name);
ST_FUNC void put_elf_reloc(uc_section_t* symtab, uc_section_t* s, unsigned long offset, int type, int symbol);
ST_FUNC void put_elf_reloca(uc_section_t* symtab, uc_section_t* s, unsigned long offset, int type, int symbol, addr_t addend);


ST_FUNC void put_stabs(const char *str, int type, int other, int desc, unsigned long value);
ST_FUNC void put_stabs_r(const char *str, int type, int other, int desc, unsigned long value, uc_section_t* sec, int sym_index);
ST_FUNC void put_stabn(int type, int other, int desc, int value);
ST_FUNC void put_stabd(int type, int other, int desc);


ST_FUNC void resolve_common_syms(uc_state_t *s1);
ST_FUNC void relocate_syms(uc_state_t *s1, uc_section_t* symtab, int do_resolve);
ST_FUNC void relocate_section(uc_state_t *s1, uc_section_t* s);


ST_FUNC int tcc_object_type(int fd, ElfW(Ehdr) *h);
ST_FUNC int tcc_load_object_file(uc_state_t *s1, int fd, unsigned long file_offset);
ST_FUNC int tcc_load_archive(uc_state_t *s1, int fd);
ST_FUNC void tcc_add_bcheck(uc_state_t *s1);
ST_FUNC void tcc_add_runtime(uc_state_t *s1);


ST_FUNC void build_got_entries(uc_state_t *s1);
ST_FUNC struct uc_symbol_attributes_x_s *get_sym_attr(uc_state_t *s1, int index, int alloc);
ST_FUNC void squeeze_multi_relocs(uc_section_t* sec, size_t oldrelocoffset);


ST_FUNC addr_t get_elf_sym_addr(uc_state_t *s, const char *name, int err);


#if defined TCC_IS_NATIVE || defined TCC_TARGET_PE
ST_FUNC void *tcc_get_symbol_err(uc_state_t *s, const char *name);
#endif


#ifndef TCC_TARGET_PE
ST_FUNC int tcc_load_dll(uc_state_t *s1, int fd, const char *filename, int level);
ST_FUNC int tcc_load_ldscript(uc_state_t *s1);
ST_FUNC uint8_t *parse_comment(uint8_t *p);
ST_FUNC void minp(void);
ST_INLN void inp(void);
ST_FUNC int handle_eob(void);
#endif


// cpu/*/lnk.c


// Whether to generate a GOT/PLT entry and when. NO_GOTPLT_ENTRY is first so
// that unknown relocation don't create a GOT or PLT entry.
enum gotplt_entry 
{
    NO_GOTPLT_ENTRY,	/* never generate (eg. GLOB_DAT & JMP_SLOT relocs) */
    BUILD_GOT_ONLY,	/* only build GOT (eg. TPOFF relocs) */
    AUTO_GOTPLT_ENTRY,	/* generate if sym is UNDEF */
    ALWAYS_GOTPLT_ENTRY	/* always generate (eg. PLTOFF relocs) */
};


ST_FUNC int code_reloc (int reloc_type);
ST_FUNC int gotplt_entry_type (int reloc_type);
ST_FUNC unsigned create_plt_entry(uc_state_t *s1, unsigned got_offset, struct uc_symbol_attributes_x_s *attr);
ST_FUNC void relocate_init(uc_section_t* sr);
ST_FUNC void relocate(uc_state_t *s1, ElfW_Rel *rel, int type, unsigned char *ptr, addr_t addr, addr_t val);
ST_FUNC void relocate_plt(uc_state_t *s1);


// cpu/*/gen.c


ST_DATA const int reg_classes[NB_REGS];


ST_FUNC void gsym_addr(int t, int a);
ST_FUNC void gsym(int t);
ST_FUNC void load(int r, uc_stack_value_t *sv);
ST_FUNC void store(int r, uc_stack_value_t *v);
ST_FUNC int gfunc_sret(uc_c_type_t *vt, int variadic, uc_c_type_t *ret, int *align, int *regsize);
ST_FUNC void gfunc_call(int nb_args);
ST_FUNC void gfunc_prolog(uc_c_type_t *func_type);
ST_FUNC void gfunc_epilog(void);
ST_FUNC int gjmp(int t);
ST_FUNC void gjmp_addr(int a);
ST_FUNC int gtst(int inv, int t);


#if defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64
ST_FUNC void gtst_addr(int inv, int a);
#else
#define gtst_addr(inv, a) gsym_addr(gtst(inv, 0), a)
#endif


ST_FUNC void gen_opi(int op);
ST_FUNC void gen_opf(int op);
ST_FUNC void gen_cvt_ftoi(int t);
ST_FUNC void gen_cvt_ftof(int t);
ST_FUNC void ggoto(void);


#ifndef TCC_TARGET_ARM
ST_FUNC void gen_cvt_itof(int t);
#endif


ST_FUNC void gen_vla_sp_save(int addr);
ST_FUNC void gen_vla_sp_restore(int addr);
ST_FUNC void gen_vla_alloc(uc_c_type_t *type, int align);


inline static uint16_t read16le(unsigned char *p) { return p[0] | (uint16_t)p[1] << 8; }
inline static void write16le(unsigned char *p, uint16_t x) { p[0] = x & 255;  p[1] = x >> 8 & 255; }
inline static uint32_t read32le(unsigned char *p) { return read16le(p) | (uint32_t)read16le(p + 2) << 16; }
inline static void write32le(unsigned char *p, uint32_t x) { write16le(p, x);  write16le(p + 2, x >> 16); }
inline static void add32le(unsigned char *p, int32_t x) { write32le(p, read32le(p) + x); }
inline static uint64_t read64le(unsigned char *p) { return read32le(p) | (uint64_t)read32le(p + 4) << 32; }
inline static void write64le(unsigned char *p, uint64_t x) { write32le(p, x);  write32le(p + 4, x >> 32); }
inline static void add64le(unsigned char *p, int64_t x) { write64le(p, read64le(p) + x); }


// cpu/x86/gen.c


#if defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64
ST_FUNC void g(int c);
ST_FUNC void gen_le16(int c);
ST_FUNC void gen_le32(int c);
ST_FUNC void gen_addr32(int r, uc_symbol_t *sym, int c);
ST_FUNC void gen_addrpc32(int r, uc_symbol_t *sym, int c);
#endif


#ifdef CONFIG_TCC_BCHECK
ST_FUNC void gen_bounded_ptr_add(void);
ST_FUNC void gen_bounded_ptr_deref(void);
#endif

// cpu/x86/64/gen.c


#ifdef TCC_TARGET_X86_64
ST_FUNC void gen_addr64(int r, uc_symbol_t *sym, int64_t c);
ST_FUNC void gen_opl(int op);
#ifdef TCC_TARGET_PE
ST_FUNC void gen_vla_result(int addr);
#endif
#endif


// cpu/arm/gen.c


#ifdef TCC_TARGET_ARM
#if defined(TCC_ARM_EABI) && !defined(CONFIG_TCC_ELFINTERP)
PUB_FUNC const char *default_elfinterp(struct uc_state_t *s);
#endif
ST_FUNC void arm_init(struct uc_state_t *s);
ST_FUNC void gen_cvt_itof1(int t);
#endif


// cpu/arm/64/gen.c


#ifdef TCC_TARGET_ARM64
ST_FUNC void gen_cvt_sxtw(void);
ST_FUNC void gen_opl(int op);
ST_FUNC void gfunc_return(uc_c_type_t *func_type);
ST_FUNC void gen_va_start(void);
ST_FUNC void gen_va_arg(uc_c_type_t *t);
ST_FUNC void gen_clear_cache(void);
#endif


// cof.c


#ifdef TCC_TARGET_COFF
ST_FUNC int tcc_output_coff(uc_state_t *s1, FILE *f);
ST_FUNC int tcc_load_coff(uc_state_t * s1, int fd);
#endif


// asm.c


ST_FUNC void asm_instr(void);
ST_FUNC void asm_global_instr(void);


#ifdef CONFIG_TCC_ASM


ST_FUNC int find_constraint(uc_assembler_operand_t *operands, int nb_operands, const char *name, const char **pp);
ST_FUNC uc_symbol_t* get_asm_sym(int name, uc_symbol_t *csym);
ST_FUNC void asm_expr(uc_state_t *s1, uc_expression_value_t *pe);
ST_FUNC int asm_int_expr(uc_state_t *s1);
ST_FUNC int tcc_assemble(uc_state_t *s1, int do_preprocess);


// cpu/x86/asm.c


ST_FUNC void gen_expr32(uc_expression_value_t *pe);


#ifdef TCC_TARGET_X86_64
ST_FUNC void gen_expr64(uc_expression_value_t *pe);
#endif

ST_FUNC void asm_opcode(uc_state_t *s1, int opcode);
ST_FUNC int asm_parse_regvar(int t);
ST_FUNC void asm_compute_constraints(uc_assembler_operand_t *operands, int nb_operands, int nb_outputs, const uint8_t *clobber_regs, int *pout_reg);
ST_FUNC void subst_asm_operand(uc_string_t *add_str, uc_stack_value_t *sv, int modifier);
ST_FUNC void asm_gen_code(uc_assembler_operand_t *operands, int nb_operands, int nb_outputs, int is_output, uint8_t *clobber_regs, int out_reg);
ST_FUNC void asm_clobber(uint8_t *clobber_regs, const char *str);


#endif // CONFIG_TCC_ASM


// pex.c


#ifdef TCC_TARGET_PE


ST_FUNC int pe_load_file(uc_state_t *s1, const char *filename, int fd);
ST_FUNC int pe_output_file(uc_state_t * s1, const char *filename);
ST_FUNC int pe_putimport(uc_state_t *s1, int dllindex, const char *name, addr_t value);


#if defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64
ST_FUNC uc_stack_value_t *pe_getimport(uc_stack_value_t *sv, uc_stack_value_t *v2);
#endif


#ifdef TCC_TARGET_X86_64
ST_FUNC void pe_add_unwind_data(unsigned start, unsigned end, unsigned stack);
#endif


PUB_FUNC int tcc_get_dllexports(const char *filename, char **pp);


// Symbol properties stored in Elf32_Sym->st_other.


#define ST_PE_EXPORT 0x10
#define ST_PE_IMPORT 0x20
#define ST_PE_STDCALL 0x40


#endif // TCC_TARGET_PE


#define ST_ASM_SET 0x04


// run.c


#ifdef TCC_IS_NATIVE


#ifdef CONFIG_TCC_STATIC

#define RTLD_LAZY       0x001
#define RTLD_NOW        0x002
#define RTLD_GLOBAL     0x100
#define RTLD_DEFAULT    NULL

// For profiling.

ST_FUNC void *dlopen(const char *filename, int flag);
ST_FUNC void dlclose(void *p);
ST_FUNC const char *dlerror(void);
ST_FUNC void *dlsym(void *handle, const char *symbol);

#endif // CONFIG_TCC_STATIC


#ifdef CONFIG_TCC_BACKTRACE
ST_DATA int rt_num_callers;
ST_DATA const char **rt_bound_error_msg;
ST_DATA void *rt_prog_main;
ST_FUNC void tcc_set_num_callers(int n);
#endif


ST_FUNC void tcc_run_free(uc_state_t *s1);


#endif // TCC_IS_NATIVE


// sup.c


#if 0


ST_FUNC int tcc_tool_ar(uc_state_t *s, int argc, char **argv);


#ifdef TCC_TARGET_PE
ST_FUNC int tcc_tool_impdef(uc_state_t *s, int argc, char **argv);
#endif


ST_FUNC void tcc_tool_cross(uc_state_t *s, char **argv, int option);
ST_FUNC void gen_makedeps(uc_state_t *s, const char *target, const char *filename);


#endif


#endif // INCLUDE_ALL_H


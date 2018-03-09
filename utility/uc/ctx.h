// ctx.h - Compiler context structure.


#ifndef INCLUDE_CTX_H
#define INCLUDE_CTX_H 1


#include <stdint.h>
#include <stdbool.h>


#include "all.h"


// Basic OS types.
typedef enum uc_os_e
{
	uc_os_undefined = 0,
	uc_os_linux,
	uc_os_windows,
}
uc_os_t;


// Processor architectures.
typedef enum uc_cpu_e
{
	uc_cpu_undefined = 0,
	uc_cpu_intel_x86,
	uc_cpu_arm,
}
uc_cpu_t;


// Processor bits.
typedef enum uc_bits_e
{
	uc_bits_undefined = 0,
	uc_bits_16 = 16,
	uc_bits_32 = 32,
	uc_bits_64 = 64,
}
uc_bits_t;


// Image types.
typedef enum uc_image_e
{
	uc_image_undefined = 0,
	uc_image_elf,
	uc_image_pe,
	uc_image_coff,
}
uc_image_t;


// Endian models.
typedef enum uc_endian_e
{
	uc_endian_undefined = 0,
	uc_endian_little,
	uc_endian_big,
	uc_endian_mid,
}
uc_endian_t;


typedef struct uc_system_s
{
	// OS.
	uc_os_t os;

	// Endian model.
	uc_endian_t endian;

	// CPU architecture.
	uc_cpu_t cpu;

	// CPU bits.
	uc_bits_t bits;

	// Image type.
	uc_image_t image;

	// CPU-specific extension flags, eg. SSE, AVX, EABI, etc.
	uint16_t cpu_extensions;
}
uc_system_t;


// Compiler state structure.
typedef struct uc_context_s
{
	// The target system.
	uc_system_t target;

	// The host / native system.
	uc_system_t native;

	// Preprocessor-specific entries.
	struct pre_s
	{
		int32_t token_flags;
		int32_t parse_flags;

		uc_buffered_file_t* file;

		int32_t character;
		int32_t token;

		uc_const_value_t token_constant;

		const int32_t* macro_pointer;

		uc_string_t current_token; // Current parsed string, if any.

		// Benchmark information.

		int32_t total_lines;
		int32_t total_bytes;
		int32_t total_identifiers;

		uc_token_symbol_t** identifier_table;
		uc_token_symbol_t* identifier_hash[UC_LIMIT_HASH_SIZE];

		char token_buffer[UC_LIMIT_STRING_MAXIMUM_SIZE + 1];

		uc_string_t string_buffer;
		uc_string_t macro_resolution_buffer;

		uc_token_string_t token_string_buffer;
		uint8_t is_id_number_table[256 - CH_EOF];

		int32_t preprocessor_debug_token;
		int32_t preprocessor_debug_symbol;
		int32_t preprocessor_once;
		int32_t preprocessor_expression;
		int32_t preprocessor_counter;

		struct uc_allocator_s* token_symbol_allocator;
		struct uc_allocator_s* token_string_allocator;
		struct uc_allocator_s* string_allocator;

		uc_token_string_t* macro_stack;
	}
	pre;

	// API-specific entries.
	struct api_s
	{
		bool use_gnu_extensions; // = true // Use GNU C extensions.
		bool use_builtin_extensions; // = true // Use built-in extensions.
		uc_state_t* this_state;
		int32_t state_count;
		void* module_handle; // Module handle.
	}
	api;

	// Code generation
	struct gen_s
	{
		// Return symbol.
		int32_t rsym;

		// Anonymous symbol index.
		int32_t anon_sym;

		// Output code index.
		int32_t ind;

		// Local variable index.
		int32_t loc;

		uc_symbol_t* sym_free_first;

		void** sym_pools;
		int32_t nb_sym_pools;

		uc_symbol_t* global_stack;
		uc_symbol_t* local_stack;
		uc_symbol_t* define_stack;
		uc_symbol_t* global_label_stack;
		uc_symbol_t* local_label_stack;

		int32_t local_scope;
		int32_t in_sizeof;
		int32_t section_sym;

		int32_t vlas_in_scope; // Number of VLAs that are currently in scope.
		int32_t vla_sp_root_loc; // The VLA stack pointer location for SP before any VLAs were pushed.
		int32_t vla_sp_loc; // Pointer to variable holding location to store stack pointer on the stack when modifying stack pointer.

		uc_stack_value_t __vstack[VSTACK_SIZE + 1];
		uc_stack_value_t* vtop;
		uc_stack_value_t* pvtop;

		bool want_constant; // True if constant wanted.
		bool want_no_code; // True if no code generation wanted.
	}
	gen;

	// Dynamic execution-specific entries.
	struct run_s
	{
		int32_t rt_num_callers; // = 6
		const char** rt_bound_error_msg;
		void* rt_prog_main;
	}
	run;

	// ELF-specific entries.
	struct elf_s
	{
		// Predefined sections.

		uc_section_t* text_section;
		uc_section_t* data_section;
		uc_section_t* bss_section; 
		uc_section_t* common_section;

		uc_section_t* cur_text_section; // Current section where function code is generated.
		uc_section_t* last_text_section; // To handle '.previous' assembler directive.
		uc_section_t* bounds_section; // Contains global data bound description.
		uc_section_t* lbounds_section; // Contains local data bound description.
		uc_section_t* symtab_section; // Symbol sections.

		// Debug sections.

		uc_section_t* stab_section; 
		uc_section_t* stabstr_section;

		bool new_undef_sym; // = false // Is there a new undefined symbol since last?
	}
	elf;
}
uc_context_t;


#endif // INCLUDE_CTX_H


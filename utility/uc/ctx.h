// ctx.h - Compiler context structure.


#ifndef INCLUDE_CTX_H
#define INCLUDE_CTX_H 1


#include <stdint.h>
#include <stdbool.h>


#include "all.h"


// Compiler state structure.
typedef struct uc_context_s
{
	// Preprocessor-specific entries.
	struct
	{
		int32_t tok_flags;
		int32_t parse_flags;

		struct uc_buffered_file_t* file;

		int32_t ch;
		int32_t tok;

		uc_const_value_t tokc;

		const int32_t* macro_ptr;

		uc_string_t tokcstr; // Current parsed string, if any.

		// Benchmark information.

		int32_t total_lines;
		int32_t total_bytes;
		int32_t tok_ident;

		uc_token_symbol_t** table_ident;

		uc_token_symbol_t* hash_ident[TOK_HASH_SIZE];
		char token_buf[STRING_MAX_SIZE + 1];

		uc_string_t cstr_buf;
		uc_string_t macro_equal_buf;

		uc_token_string_t tokstr_buf;
		uint8_t isidnum_table[256 - CH_EOF];

		int32_t pp_debug_tok;
		int32_t pp_debug_symv;
		int32_t pp_once;
		int32_t pp_expr;
		int32_t pp_counter;

		struct TinyAlloc* toksym_alloc;
		struct TinyAlloc* tokstr_alloc;
		struct TinyAlloc* cstr_alloc;

		uc_token_string_t* macro_stack;
	}
	pre;

	// API-specific entries.
	struct
	{
		bool gnu_ext; // = true // Use GNU C extensions.
		bool tcc_ext; // = true // Use built-in extensions.

		struct uc_state_t* tcc_state;

		int32_t nb_states;
		void* tcc_module; // Module handle.
	}
	api;

	// Code generation
	struct
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
		int32_t vla_sp_root_loc; // The vla_sp_loc for SP before any VLAs were pushed.
		int32_t vla_sp_loc; // Pointer to variable holding location to store stack pointer on the stack when modifying stack pointer.

		uc_stack_value_t __vstack[VSTACK_SIZE + 1];
		uc_stack_value_t* vtop;
		uc_stack_value_t* pvtop;

		bool const_wanted; // True if constant wanted.
		bool nocode_wanted; // True if no code generation wanted.
	}
	gen;

	// Dynamic execution-specific entries.
	struct
	{
		int32_t rt_num_callers; // = 6
		const char** rt_bound_error_msg;
		void* rt_prog_main;
	}
	run;

	// ELF-specific entries.
	struct
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


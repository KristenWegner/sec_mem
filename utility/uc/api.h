// api.h - Compiler API.


#ifndef INCLUDE_API_H
#define INCLUDE_API_H 1


#ifndef LIBTCCAPI
#define LIBTCCAPI
#endif


struct uc_state_s;


// Create a new compilation context.
LIBTCCAPI uc_state_t* tcc_new(void);


// Destroy a compilation context.
LIBTCCAPI void tcc_delete(uc_state_t *s);


// Set CONFIG_TCCDIR at runtime.
LIBTCCAPI void tcc_set_lib_path(uc_state_t *s, const char *path);


// Set error / warning callback.
LIBTCCAPI void tcc_set_error_func(uc_state_t *s, void *error_opaque, void (*error_func)(void *opaque, const char *msg));


// Set options as from command line.
LIBTCCAPI void tcc_set_options(uc_state_t *s, const char *str);


// Preprocessor.


// Add include path.
LIBTCCAPI int tcc_add_include_path(uc_state_t *s, const char *pathname);


// Add in system include path.
LIBTCCAPI int tcc_add_sysinclude_path(uc_state_t *s, const char *pathname);


// Define preprocessor symbol with optional value.
LIBTCCAPI void uc_define_symbol(uc_state_t *s, const char *sym, const char *value);


// Undefine uc_preprocess symbol.
LIBTCCAPI void tcc_undefine_symbol(uc_state_t *s, const char *sym);


// Compilation.


// Adds a file (source file, dll, object, library, ld script) to the build. Returns -1 on error.
LIBTCCAPI int tcc_add_file(uc_state_t *s, const char *filename);


// Compiles a source string. Returns -1 on error.
LIBTCCAPI int tcc_compile_string(uc_state_t *s, const char *buf);


// Linker.


// Output types.


#define TCC_OUTPUT_MEMORY     1 // Output will be run in memory (default).
#define TCC_OUTPUT_EXE        2 // Executable file.
#define TCC_OUTPUT_DLL        3 // Dynamic library.
#define TCC_OUTPUT_OBJ        4 // Object file.
#define TCC_OUTPUT_PREPROCESS 5 // Only uc_preprocess (used internally).


// Set output type. Note: This must be called before compilation.
LIBTCCAPI int tcc_set_output_type(uc_state_t *s, int output_type);


// Equivalent to -Lpath option.
LIBTCCAPI int tcc_add_library_path(uc_state_t *s, const char *pathname);


// The library name is the same as the argument of the '-l' option.
LIBTCCAPI int tcc_add_library(uc_state_t *s, const char *libraryname);


// Adds a symbol to the compiled program.
LIBTCCAPI int tcc_add_symbol(uc_state_t *s, const char *name, const void *val);


// Outputs an executable, library or object file. Do not call tcc_relocate() before this.
LIBTCCAPI int tcc_output_file(uc_state_t *s, const char *filename);


// Link and run main() function and return it's value. Do not call tcc_relocate() before this.
LIBTCCAPI int tcc_run(uc_state_t *s, int argc, char **argv);


// Possible values for relocation:
//	TCC_RELOCATE_AUTO	: Allocate and manage memory internally.
//	NULL				: Returns the required memory size for the step below.
//	Address				: Copies code to memory passed by the caller.
#define TCC_RELOCATE_AUTO ((void*)1)


// Do all relocations, needed before using tcc_get_symbol(). Returns -1 on error.
LIBTCCAPI int tcc_relocate(uc_state_t *s1, void *ptr);


// Returns symbol value or NULL if not found.
LIBTCCAPI void *tcc_get_symbol(uc_state_t *s, const char *name);


#endif // INCLUDE_API_H


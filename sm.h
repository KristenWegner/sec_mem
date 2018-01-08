// sm.h - Secure memory toolkit API heaqder.


#include "config.h"


#ifndef INCLUDE_SM_H
#define INCLUDE_SM_H 1


// Types


// Context type.
typedef void* sm_t;

// Error code type for sm_err_f.
typedef uint16_t sm_error_t;

// Opcode type for sm_op.
typedef uint16_t sm_opcode_t;

// Reference type. Returned by sm_get_entity.
typedef uint64_t sm_ref_t;


// Function Types


// Function that accepts no arguments and returns a 64-bit integer.
typedef uint64_t (*sm_get64_f)(void);

// Seeds a RNG state vector with the given seed.
typedef void (*sm_srs64_f)(void* state, uint64_t seed);

// Gets a random 64-bit integer, given the specified state vector.
typedef uint64_t (*sm_ran64_f)(void* state);

// Compute 64-bit CRC with starting seed, data array, size in bytes, and tab.
typedef uint64_t (*sm_crc64_f)(uint64_t seed, const uint8_t* data, uint64_t size, void* tab);

// Compute 32-bit CRC with starting seed, data array, size in bytes, and tab.
typedef uint32_t (*sm_crc32_f)(uint32_t seed, const uint8_t* data, uint64_t size, void* tab);

// Compress from src to dst. Size of temporary param htab must be determined by sm_op_get_size_compress_htab. Returns 0 on no error.
typedef uint8_t (*sm_cpr_f)(const void *const src, const uint64_t slen, void *dst, uint64_t *const dlen, void* htab);

// Decompress from src to dst. Returns 0 on no error. If dst is null and *dlen is zero, will set *dlen to to required dst buffer size.
typedef uint8_t (*sm_dcp_f)(const void* src, uint64_t slen, void* dst, uint64_t* dlen);

// 64-bit hash bytes.
typedef uint64_t (*sm_hsh64_f)(const void*, size_t);

// 32-bit hash bytes.
typedef uint32_t (*sm_hsh32_f)(const void*, size_t);

// Error callback function type. Receives an error code - see: SM_ERR_*.
typedef void (*sm_err_f)(sm_t sm, sm_error_t);

// Mutex operational function.
typedef uint8_t (*sm_mutex_f)(sm_mutex_t*);


// Methods


// Master entropic/quasi-entropic random number generator. Uses RDRAND if 
// present, else uses XorShift1024* 64-bit with random intermittent 
// re-seeding. If sm_t is null, uses default RNG support.
extern uint64_t sm_random(sm_t);

// Creates a new context with the initial count of space in bytes.
extern sm_t callconv sm_create(uint64_t bytes);

// Destroys the specified context.
extern void callconv sm_destroy(sm_t);

// Set the error handler for the specified context.
extern void callconv sm_set_error_handler(sm_t* sm, sm_err_f handler);

// Get protected entity.
extern sm_ref_t callconv sm_get_entity(sm_t* sm, uint16_t op);


// Error Codes


#define SM_ERR_NOT_INITIALIZED		(1 << 1) // System not initialized.
#define SM_ERR_NO_SUCH_COMMAND		(1 << 2) // No such command.
#define SM_ERR_INVALID_ARGUMENT		(1 << 3) // Invalid argument(s).
#define SM_ERR_INVALID_POINTER		(1 << 4) // Invalid or null pointer/reference.
#define SM_ERR_INVALID_CRC			(1 << 5) // Invalid CRC encountered.
#define SM_ERR_OUT_OF_MEMORY		(1 << 6) // Out of memory or allocation failed.
#define SM_ERR_CANNOT_MAKE_EXEC		(1 << 7) // Failed to make memory page executable.


// Built-In Opcodes


#define SM_OP_CREATE				(0x0001) // Create/initialize the global state.
#define SM_OP_DESTROY				(0x0002) // Destroy/uninitialize the global state.
#define SM_OP_SET_ERROR_HANDLER		(0x0003) // Set the error handler.
#define SM_OP_ALLOCATE				(0x0004) // Allocate memory.
#define SM_OP_REALLOCATE			(0x0005) // Reallocate memory.
#define SM_OP_FREE					(0x0006) // Free memory.
#define SM_OP_UNLOCK				(0x0007) // 
#define SM_OP_LOCK					(0x0008) // 
#define SM_OP_CREATE_ARRAY			(0x0009) // 
#define SM_OP_DESTROY_ARRAY			(0x000A)
#define SM_OP_ARRAY_ELEMENT_GET		(0x000B)
#define SM_OP_ARRAY_ELEMENT_SET		(0x000C)
#define SM_OP_ARRAY_SIZE_GET		(0x000D)
#define SM_OP_ARRAY_SIZE_SET		(0x000E)


// Add additional opcodes declarations here.
#if !defined(DEBUG) && !defined(_DEBUG)
#include "precursors/rdr_decl.h"
#include "precursors/crc_decl.h"
#include "precursors/ran_decl.h"
#endif

#endif // INCLUDE_SM_H



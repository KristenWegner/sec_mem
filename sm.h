// sm.h - Secure memory toolkit API heaqder.


#include "config.h"


#ifndef INCLUDE_SM_H
#define INCLUDE_SM_H 1


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

// Error code type for sm_err_f.
typedef uint16_t sm_error_t;

// Error callback function type. Receives an error code - see: SM_ERR_*.
typedef void (*sm_err_f)(sm_error_t);

// Opcode type for sm_op.
typedef uint16_t sm_opcode_t; 

// Reference type. Returned by sm_op.
typedef uint64_t sm_ref_t;


////////////////////////////////////////////////////////////////////////////////
// Master entropic/quasi-entropic random number generator. Uses RDRAND if 
// present, else uses XorShift1024* 64-bit with random intermittent 
// re-seeding.
////////////////////////////////////////////////////////////////////////////////
extern uint64_t sm_master_rand();


////////////////////////////////////////////////////////////////////////////////
// In-place transcode (scramble/unscramble) data of count bytes using sequence 
// with state seeded by key. Transcode encode and transcode decode are 
// symmetrical given the same key, and random seed/generation function 
// pair.
//
// Parameters:
//
//     - encode: If true then encode, else decode.
//     - data: Pointer to data to encode.
//     - bytes: Count of bytes to starting at data.
//     - key: The random key (seed).
//     - state: Pointer to random state vector.
//     - size: Random state vector size in bytes.
//     - seed: Pointer to random seed function.
//     - random: Pointer to random generate function.
// 
// Returns a pointer to the start of the transcoded buffer.
////////////////////////////////////////////////////////////////////////////////
extern void* callconv sm_transcode(uint8_t encode, void *restrict data, register size_t bytes, uint64_t key, void* state, size_t size, void(*seed)(void*, uint64_t), uint64_t(*random)(void*));


////////////////////////////////////////////////////////////////////////////////
// Do a single XOR pass on the given data using XorShift1024* 64-bit values
// generated using the given seed (key).
////////////////////////////////////////////////////////////////////////////////
extern void* sm_xor_pass(void *restrict data, register size_t bytes, uint64_t key);


////////////////////////////////////////////////////////////////////////////////
// Cross XOR using XorShift1024* 64-bit, decoding into dst using key1 (the 
// first seed), while byte by byte recoding src again using key2 (the second 
// seed). Returns dst.
////////////////////////////////////////////////////////////////////////////////
extern void* sm_xor_cross(void *restrict dst, void *restrict src, register size_t bytes, uint64_t key1, uint64_t key2);


////////////////////////////////////////////////////////////////////////////////
// Nexus command. See SM_OP_*.
////////////////////////////////////////////////////////////////////////////////
extern sm_ref_t sm_op(sm_opcode_t op, uint64_t arg1, uint64_t arg2, uint64_t arg3);


// Error Codes


#define SM_ERR_NOT_INITIALIZED		(1 << 1) // System not initialized.
#define SM_ERR_NO_SUCH_COMMAND		(1 << 2) // No such command.
#define SM_ERR_INVALID_ARGUMENT		(1 << 3) // Invalid argument(s).
#define SM_ERR_INVALID_POINTER		(1 << 4) // Invalid or null pointer/reference.
#define SM_ERR_INVALID_CRC			(1 << 5) // Invalid CRC encountered.
#define SM_ERR_OUT_OF_MEMORY		(1 << 6) // Out of memory or allocation failed.


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


#endif // INCLUDE_SM_H



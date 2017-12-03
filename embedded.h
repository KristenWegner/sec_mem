// embedded.h - Embedded function access.


#include "config.h"


#ifndef INCLUDE_EMBEDDED_H
#define INCLUDE_EMBEDDED_H 1


// Opcodes


#define SEC_OP_HRDRND64 (0x4300U) // Tests for rdrand support: sec_g64_f.
#define SEC_OP_RDRAND64 (0x9ADFU) // Read rand via rdrand: sec_g64_f.

#define SEC_OP_SHRGML64 (0xF661U) // Multiply using Schrage's method: sec_sch_t;

#define SEC_OP_FS20SD64 (0x1888U) // Fishman-20 64 RNG seed: sec_srs_f.
#define SEC_OP_FS20RG64 (0xBE56U) // Fishman-20 64 RNG generate: sec_r64_f.
#define SEC_OP_FS20SS64 (0xCC906) // Fishman-20 64 RNG state size, uint64_t.

#define SEC_OP_KN02SD64 (0x046BU) // Knuth 2002 64 With Random Bit Shift RNG seed: sec_srs_f.
#define SEC_OP_KN02RG64 (0x7BC6U) // Knuth 2002 64 With Random Bit Shift RNG generate: sec_r64_f.
#define SEC_OP_KN02SS64 (0xD549U) // Knuth 2002 64 With Random Bit Shift RNG state size: uint64_t.

#define SEC_OP_LECUSR32 (0x1CEAU) // L'Ecuyer 32 RND seed: sec_srs_f.
#define SEC_OP_LECURG32 (0xE92EU) // L'Ecuyer 32 RNG generate: sec_r32_f.
#define SEC_OP_LECUSS32 (0xD7ADU) // L'Ecuyer 32 RNG state size, uint64_t.

#define SEC_OP_GFSRSR32 (0x5AF3U) // GFSR4 32 RND seed: sec_srs_f.
#define SEC_OP_GFSRRG32 (0xE998U) // GFSR4 32 RNG generate: sec_r32_f.
#define SEC_OP_GFSRSS32 (0xFE41U) // GFSR4 32 RNG state size, uint64_t.

#define SEC_OP_SPMXSR64 (0xA9C9U) // Split Mix 64 RNG seed: sec_srs_f.
#define SEC_OP_SPMXRG64 (0xC6C8U) // Split Mix 64 RNG generate: sec_r64_f.
#define SEC_OP_SPMXSS64 (0xF5B7U) // Split Mix 64 RNG state size, uint64_t.

#define SEC_OP_XSHISR64 (0x73DBU) // Xoroshiro128+ 64 RNG seed: sec_srs_f.
#define SEC_OP_XSHIRG64 (0x515BU) // Xoroshiro128+ 64 RNG generate: sec_r64_f.
#define SEC_OP_XSHISS64 (0xC06CU) // Xoroshiro128+ 64 RNG state size, uint64_t.

#define SEC_OP_XSFSSR64 (0xCD0FU) // XorShift1024* 64 RNG seed: sec_srs_f.
#define SEC_OP_XSFSRG64 (0x43CFU) // XorShift1024* 64 RNG generate: sec_r64_f.
#define SEC_OP_XSFSSS64 (0xCF7CU) // XorShift1024* 64 RNG state size, uint64_t.

#define SEC_OP_MERSSR64 (0xC889U) // Mersenne Twister 19937 64 RNG seed: sec_srs_f.
#define SEC_OP_MERSRG64 (0x403EU) // Mersenne Twister 19937 64 RNG generate: sec_r64_f.
#define SEC_OP_MERSSS64 (0x1843U) // Mersenne Twister 19937 64 RNG state size, uint64_t.

#define SEC_OP_COMPHTAB (0xDBCBU) // Get the HTAB size needed by SEC_OP_COMPRESS: uint64_t.
#define SEC_OP_COMPRESS (0x22E8U) // Compress data: sec_cpr_f.
#define SEC_OP_DECOMPRS (0x7EE1U) // Decompress data: sec_dcp_f.

#define SEC_OP_CRC64FUN (0x08B6U) // CRC 64: sec_c64_f.
#define SEC_OP_CRC64TAB (0x1B33U) // Get the CRC 64 standard LUT for SEC_OP_CRC64FUN: void*.

#define SEC_OP_CRC32FUN (0x2945U) // CRC 32: sec_c32_f.
#define SEC_OP_CRC32TAB (0x45FAU) // Get the CRC 32 standard LUT for SEC_OP_CRC32FUN: void*.

#define SEC_OP_MEMCPYFN (0x82EDU) // Same as memcpy: sec_cpy_f.
#define SEC_OP_MEMMEMFN (0x0DD6U) // Same as memmem: sec_mem_f.
#define SEC_OP_MEMXORRG (0xDA82U) // Memory XOR given RNG function and state: sec_mxr_f.

#define SEC_OP_HCITYB64 (0x8FF1U) // City Hash 64-Bit, up to 8 bytes: sec_h64_f.
#define SEC_OP_HMURMU64 (0xADC2U) // Murmur Hash 64-Bit: sec_h64_f.
#define SEC_OP_HMURMU32 (0x3FE3U) // Murmur Hash 32-Bit: sec_h32_f.


// Gets the specified element, or zero. If the request is for a procedure, it must be freed when no longer needed.
uint64_t sec_op(uint16_t);


// Function Types


// Multiply using Schrage's method.
typedef uint64_t (*sec_sch_t)(uint64_t, uint64_t, uint64_t, uint64_t);

// Function that accepts no arguments and returns a 64-bit integer.
typedef uint64_t (*sec_g64_f)(void);

// Seeds the specified RNG state vector with the given seed.
typedef void (*sec_srs_f)(void* state, uint64_t seed);

// Gets a random 64-bit integer in [0..0xFFFFFFFFFFFFFFFF], given the specified state vector.
typedef uint64_t (*sec_r64_f)(void* state);

// Gets a random 32-bit integer in [0..0xFFFFFFFF], given the specified state vector.
typedef uint32_t (*sec_r32_f)(void* state);

// Compute 64-bit CRC with starting seed, data array, size in bytes, and tab. Call SEC_OP_CRC64TAB to get standard tab.
typedef uint64_t (*sec_c64_f)(uint64_t seed, const uint8_t* data, uint64_t size, void* tab);

// Compute 32-bit CRC with starting seed, data array, size in bytes, and tab. Call SEC_OP_CRC32TAB to get standard tab.
typedef uint32_t (*sec_c32_f)(uint32_t seed, const uint8_t* data, uint64_t size, void* tab);

// Compress from src to dst. Size of temporary param htab must be determined by SEC_OP_COMPHTAB. Returns 0 on no error.
typedef uint8_t (*sec_cpr_f)(const void *const src, const uint64_t slen, void *dst, uint64_t *const dlen, void* htab);

// Decompress from src to dst. Returns 0 on no error. If dst is null and *dlen is zero, will set *dlen to to required dst buffer size.
typedef uint8_t (*sec_dcp_f)(const void* src, uint64_t slen, void* dst, uint64_t* dlen);

// Performs ptr[i] = ptr[i] ^ ran(state), for the given count of bytes (len). Returns ptr.
typedef void* (*sec_mxr_f)(void* ptr, uint64_t len, sec_r64_f ran, void* state);

// Returns the first occurrence of q of length n in p of length m. If not found, or m less than n, returns null. Returns p if n is zero.
typedef void* (*sec_mem_f)(const void* p, size_t m, const void* q, size_t n);

// Copies n bytes from memory region q to region p. Returns p.
typedef void* (*sec_cpy_f)(void* p, const void* q, size_t n);

// 64-bit hash bytes.
typedef uint64_t (*sec_h64_f)(const void*, size_t);

// 32-bit hash bytes.
typedef uint32_t (*sec_h32_f)(const void*, size_t);


#endif // INCLUDE_EMBEDDED_H


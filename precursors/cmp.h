// cmp.h

#include "../config.h"

#ifndef INCLUDE_CMP_H
#define INCLUDE_CMP_H 1

#define SEC_CMP_RC_OK        0 // Success.
#define SEC_CMP_RC_SIZE      1 // Output buffer too small.
#define SEC_CMP_RC_CORRUPT   2 // Invalid data for decompression.
#define SEC_CMP_RC_ARGUMENTS 3 // Arguments invalid.

// Compress from src to dst. Param htab must be uint8_t* ht[0x10000].
uint8_t __stdcall sec_compress(const void *const src, const uint64_t slen, void *dst, uint64_t *const dlen, void* htab);

// Decompress from src to dst. If dst is null and *dlen is zero, will set *dlen to to required dst buffer size.
uint8_t __stdcall sec_decompress(const void* src, uint64_t slen, void* dst, uint64_t* dlen);


#endif // INCLUDE_CMP_H


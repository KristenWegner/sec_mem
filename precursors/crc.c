// crc.c - 32 and 64-bit CRC functions.


#include "../config.h"


#define exported // Just to identify exported functions.


#if defined(SEC_OS_WINDOWS)
#define callconv __stdcall // Do not emit extra prologue instructions.
#elif defined(SEC_OS_LINUX)
#define callconv __attribute__((stdcall))
#else
#define callconv
#endif


exported uint32_t callconv CRC32FUN(uint32_t c, register const uint8_t *restrict p, uint64_t n, void* t)
{
	register const uint32_t* tab = t;
	c = c ^ ~0U;
	while (n--)
		c = tab[(c ^ *p++) & 0xFF] ^ (c >> 8);
	return c ^ ~0U;
}

exported uint64_t callconv CRC64FUN(uint64_t c, register const uint8_t *restrict p, uint64_t n, void* t)
{
	register const uint64_t* tab = t;
	while (n--)
		c = tab[(uint8_t)c ^ *p++] ^ (c >> 8);
	return c;
}


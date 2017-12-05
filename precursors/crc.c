// crc.c - 32 and 64-bit CRC functions.


#include "../config.h"


#define EXPORTED // Just to identify exported functions.


#if defined(SEC_OS_WINDOWS)
#define CALLCONV __stdcall // Do not emit extra prologue instructions.
#elif defined(SEC_OS_LINUX)
#define CALLCONV __attribute__((stdcall))
#else
#define CALLCONV
#endif


EXPORTED uint32_t CALLCONV CRC32FUN(uint32_t c, register const uint8_t *restrict p, uint64_t n, void* t)
{
	register const uint32_t* tab = t;
	c = c ^ ~0U;
	while (n--)
		c = tab[(c ^ *p++) & 0xFF] ^ (c >> 8);
	return c ^ ~0U;
}

EXPORTED uint64_t CALLCONV CRC64FUN(uint64_t c, register const uint8_t *restrict p, uint64_t n, void* t)
{
	register const uint64_t* tab = t;
	while (n--)
		c = tab[(uint8_t)c ^ *p++] ^ (c >> 8);
	return c;
}


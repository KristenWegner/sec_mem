
#include "config.h"


uint32_t __stdcall CRC32FUN(uint32_t c, register const uint8_t *restrict p, uint64_t n, void* t)
{
	register const uint32_t* tab = t;
	c = c ^ ~0U;
	while (n--)
		c = tab[(c ^ *p++) & 0xFF] ^ (c >> 8);
	return c ^ ~0U;
}

uint64_t __stdcall CRC64FUN(uint64_t c, register const uint8_t *restrict p, uint64_t n, void* t)
{
	register const uint64_t* tab = t;
	while (n--)
		c = tab[(uint8_t)c ^ *p++] ^ (c >> 8);
	return c;
}


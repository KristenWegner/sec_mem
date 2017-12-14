// crc.c - 32 and 64-bit CRC functions.


#include "../config.h"


exported uint32_t callconv sm_crc_32(uint32_t c, register const uint8_t *restrict p, uint64_t n, void* t)
{
	register const uint32_t* tab = t;
	c = c ^ ~UINT32_C(0);
	while (n--)
		c = tab[(c ^ *p++) & UINT32_C(0xFF)] ^ (c >> 8);
	return c ^ ~UINT32_C(0);
}


exported uint64_t callconv sm_crc_64(uint64_t c, register const uint8_t *restrict p, uint64_t n, void* t)
{
	register const uint64_t* tab = t;
	while (n--)
		c = tab[(uint8_t)c ^ *p++] ^ (c >> 8);
	return c;
}


// checksum.h


#include "config.h"


#ifndef INCLUDE_CHECKSUM_H
#define INCLUDE_CHECKSUM_H 1


#define CRCPOLY 0xEDB88320
#define CRCINIT 0xFFFFFFFF


void sec_crc_initialize(void *restrict s)
{
	register uint32_t i, x, j, c;
	register uint32_t* p = (uint32_t*)s;

	for (i = UINT32_C(0); i <= UINT32_C(0xFF); ++i)
	{
		x = i;

		for (j = 0; j < UINT32_C(8); ++j)
			x = (x >> 1) ^ (UINT32_C(0xEDB88320) & (-(int32_t)(x & UINT32_C(1))));

		p[(0 * 256) + i] = x;
	}

	for (i = UINT32_C(0); i <= UINT32_C(0xFF); ++i)
	{
		c = p[(0 * 256) + i];

		for (j = 1; j < UINT32_C(4); ++j)
		{
			c = p[(0 * 256) + (c & UINT32_C(0xFF))] ^ (c >> 8);
			p[(j * 256) + i] = c;
		}
	}
}

uint32_t sec_crc_compute(const void *restrict s, const uint8_t *restrict v, size_t n)
{
	uint32_t r = UINT32_C(0xFFFFFFFF);
	uint32_t i = (uint32_t)(n / sizeof(uint32_t));
	uint32_t c;

	for (; i; i--) 
	{
		r ^= *(uint32_t*)v;
		r =
			g_crc_precalc[3][(r) & 0xFF] ^
			g_crc_precalc[2][(r >> 8) & 0xFF] ^
			g_crc_precalc[1][(r >> 16) & 0xFF] ^
			g_crc_precalc[0][(r >> 24) & 0xFF];
		v += sizeof(uint32_t);
	}
	if (n & sizeof(uint16_t)) {
		c = r ^ *(uint16_t*)v;
		r = g_crc_precalc[1][(c) & 0xFF] ^
			g_crc_precalc[0][(c >> 8) & 0xFF] ^ (r >> 16);
		v += sizeof(uint16_t);
	}

	if (n & 1)
		r = g_crc_precalc[0][(r ^ *v) & 0xFF] ^ (r >> 8);

	return ~r;
}


#endif // 


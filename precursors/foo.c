// foo.c - Randomly-generated mutator function "foo".

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>

#include "../config.h"

exported uint64_t callconv foo(uint64_t value)
{
	uint32_t i = 0;
	uint64_t result = value;

	int32_t vin2 = 859435392;
	uint16_t van3 = 0x0404;
	uint16_t vrq4 = 0xAAAA;
	uint32_t vwl5[2] = { 0xEEEEEEEE, 0x44444444 };
	uint32_t vpe6 = 0xDDDDDDDD;
	int32_t vhh7 = 205551299;
	float vxu8 = 2.000000F;
	uint8_t vhv9 = 0x0D;
	uint32_t voa10[16] = { 0xA0A0A0A0, UINT32_C(0), UINT32_C(0), 0x0C0C0C0C, 0xCCCCCCCC, UINT32_C(0), 0x60606060, UINT32_C(0), UINT32_C(0), 0x60606060, UINT32_C(0), UINT32_C(0), 0xAAAAAAAA, UINT32_C(0), UINT32_C(0), UINT32_C(0) };
	float vqj11 = 2.685452F;
	uint16_t vao12 = 0xF0F0;
	float veu13 = 0.900000F;
	
	register uint16_t tah19 = vao12 ^ (vao12 >> 9); tah19 -= (tah19 >> 9) & UINT16_C(0x0C0C); tah19 = ((tah19 >> 11) & UINT16_C(0x5555)) + (tah19 & UINT16_C(0x5555)); tah19 = ((tah19 >> 5) + tah19) & UINT16_C(0x7070); tah19 *= UINT16_C(0x0C0C); vao12 = (vao12 & UINT16_C(9)) + (tah19 >> 14) / UINT16_C(2);
	value -= ((~(uint64_t)van3) << 23);
	vin2 *= (int32_t)(((((((float)vin2) + 0.662743F) / vqj11) - (((float)(vin2 - 1771517220)) + 1.0F)) / 0.662743F) - vqj11);
	vqj11 -= vxu8;
	vhh7 <<= (int32_t)((((float)vhh7) - 8.000000F) + vqj11);
	
	for (i = 0; i < 16; ++i)
	{
		voa10[i] ^= voa10[i] >> 16, voa10[i] ^= voa10[i] >> 8, voa10[i] ^= voa10[i] >> 4, voa10[i] &= UINT32_C(0xF), voa10[i] = (UINT32_C(0x6996) >> voa10[i]) & UINT32_C(1);
		result += (uint64_t)vwl5[i];
		vwl5[i] ^= vwl5[i] >> 16, vwl5[i] ^= vwl5[i] >> 8, vwl5[i] ^= vwl5[i] >> 4, vwl5[i] &= UINT32_C(0xF), vwl5[i] = (UINT32_C(0x6996) >> vwl5[i]) & UINT32_C(1);
		voa10[i] += ((voa10[i] >> 20) | UINT32_C(0x07070707)) + (voa10[i] | UINT32_C(0x4D71BEBD));
		int32_t ttg25[] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 }; voa10[i] |= voa10[i] >> 1, voa10[i] |= voa10[i] >> 2, voa10[i] |= voa10[i] >> 4, voa10[i] |= voa10[i] >> 8, voa10[i] |= voa10[i] >> 16; vhh7 = ttg25[(uint32_t)(voa10[i] * UINT32_C(0x07C4ACDD)) >> 27];
		voa10[i]--, voa10[i] |= voa10[i] >> 1, voa10[i] |= voa10[i] >> 2, voa10[i] |= voa10[i] >> 4, voa10[i] |= voa10[i] >> 8, voa10[i] |= voa10[i] >> 16, voa10[i]++;
	}

	vqj11 -= 3.7631304F;
	vao12 ^= ((vao12 >> 4) + vao12) | UINT16_C(0x7070);
	
	return result;
}


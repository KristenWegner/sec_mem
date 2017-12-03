// mt.c


#include "../config.h"


#define mt_maximum 0xFFFFFFFFULL
#define mt_minimum 0ULL
#define mt_state_size (sizeof(int32_t) + (sizeof(uint64_t) * 0x0138U))


void __stdcall mt_seed(void* s, uint64_t seed)
{
	register int32_t *i = s, j;
	register uint64_t* m = (uint64_t*)&i[1];

	m[0] = seed;

	for (j = 1; j < 0x0138; ++j)
		m[j] = (0x5851F42D4C957F2DULL * (m[j - 1] ^ (m[j - 1] >> 62)) + j);

	*i = 0;
}


uint64_t __stdcall mt_get(void* s)
{
	const uint64_t mag[2] = { 0ULL, 0xB5026F5AA96619E9ULL };
	register int32_t *i = s, j;
	register uint64_t x, *m = (uint64_t*)&i[1];

	if (*i >= 0x0138) 
	{
		if (*i == 0x0139)
			mt_seed(s, 5489ULL);

		for (j = 0; j < 0x9C; ++j)
		{
			x = (m[j] & 0xFFFFFFFF80000000ULL) | (m[j + 1] & 0x7FFFFFFFULL);
			m[j] = m[j + 0x9C] ^ (x >> 1) ^ mag[(int32_t)(x & 1ULL)];
		}

		for (; j < 0x0137; ++j) 
		{
			x = (m[j] & 0xFFFFFFFF80000000ULL) | (m[j + 1] & 0x7FFFFFFFULL);
			m[j] = m[j + (0x9C - 0x0138)] ^ (x >> 1) ^ mag[(int32_t)(x & 1ULL)];
		}

		x = (m[0x0137] & 0xFFFFFFFF80000000ULL) | (m[0] & 0x7FFFFFFFULL);
		m[0x0137] = m[0x9B] ^ (x >> 1) ^ mag[(int32_t)(x & 1ULL)];

		*i = 0;
	}

	j = *i;

	x = m[j];

	x ^= (x >> 29) & 0x5555555555555555ULL;
	x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
	x ^= (x << 37) & 0xFFF7EEE000000000ULL;
	x ^= (x >> 43);

	*i = *i + 1;

	return x;
}
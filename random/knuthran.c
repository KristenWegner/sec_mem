// knuthran.c


#include "../config.h"


#define knuthran_maximum 0x3FFFFFFFULL
#define knuthran_minimum 0ULL
#define knuthran_state_size (sizeof(uint32_t) + (sizeof(uint64_t) * 0x07D9) + (sizeof(uint64_t) * 0x64))


inline static uint64_t knuthran_get(void* state)
{
	uint32_t* j = state;
	uint64_t* a = (uint64_t*)&j[1];
	uint64_t* x = &a[0x07D9];
	uint32_t i = *j, k, l;
	if (i == 0)
	{
		for (l = 0; l < 0x64; ++l) a[l] = x[l];
		for (; l < 0x07D9; ++l) a[l] = (a[l - 0x64] - a[l - 0x25]) & (0x40000000 - 1);
		for (k = 0; k < 0x25; ++k, ++l) x[k] = (a[l - 0x64] - a[l - 0x25]) & (0x40000000 - 1);
		for (; k < 0x64; ++k, ++l) x[k] = (a[l - 0x64] - x[k - 0x25]) & (0x40000000 - 1);
	}
	*j = (i + 1) % 0x07D9;
	return a[i];
}


inline static void knuthran_seed(void* state, uint64_t seed)
{
	uint32_t* k = state;
	uint64_t *a = (uint64_t*)&k[1], *y = &a[0x07D9];
	int64_t x[0x64 + 0x64 - 1];
	register int32_t j, t;
	register int64_t s = (seed + 2) & (0x40000000 - 2);

	for (j = 0; j < 0x64; ++j)
	{
		x[j] = s;
		s <<= 1;
		if (s >= 0x40000000) 
			s -= 0x40000000 - 2;
	}

	for (; j < 0x64 + 0x64 - 1; ++j) x[j] = 0;

	x[1]++;
	s = seed & (0x40000000 - 1);
	t = 0x46 - 1;

	while (t)
	{
		for (j = 0x64 - 1; j > 0; --j) x[j + j] = x[j];
		for (j = 0x64 + 0x64 - 2; j > 0x64 - 0x25; j -= 2)
			x[0x64 + 0x64 - 1 - j] = (x[j] & (0x40000000 - 2));

		for (j = 0x64 + 0x64 - 2; j >= 0x64; --j)
		{
			if (x[j] & 1)
			{
				x[j - (0x64 - 0x25)] = (x[j - (0x64 - 0x25)] - x[j]) & (0x40000000 - 1);
				x[j - 0x64] = (x[j - 0x64] - x[j]) & (0x40000000 - 1);
			}
		}

		if (s & 1)
		{
			for (j = 0x64; j > 0; --j) x[j] = x[j - 1];
			x[0] = x[0x64];
			if (x[0x64] & 1) x[0x25] = (x[0x25] - x[0x64]) & (0x40000000 - 1);
		}

		if (s) s >>= 1;
		else t--;
	}

	*k = 0;
	for (j = 0; j < 0x25; j++) y[j + 0x64 - 0x25] = x[j];
	for (; j < 0x64; j++) y[j - 0x25] = x[j];
}

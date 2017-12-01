// knuthran2002.c


#include "../config.h"


#define knuthran2002_maximum 0x3FFFFFFFULL
#define knuthran2002_minimum 0ULL
#define knuthran2002_state_size (sizeof(uint32_t) + (sizeof(int64_t) * 0x03F1U) + (sizeof(int64_t) * 0x64U))


inline static uint64_t knuthran2002_get(void *restrict state)
{
	register uint32_t *j = state, i = *j, k, l;
	register int64_t* a = (int64_t*)&j[1U];
	register int64_t* x = &a[0x03F1ULL];
	uint64_t v;

	if (i == 0U) 
	{
		for (l = 0; l < 0x64U; ++l) a[l] = x[l];
		for (; l < 0x03F1ULL; ++l) a[l] = (a[l - 0x64U] - a[l - 0x25U]) & 0x3FFFFFFFULL;
		for (k = 0; k < 0x25U; ++k, ++l) x[k] = (a[l - 0x64U] - a[l - 0x25U]) & 0x3FFFFFFFULL;
		for (; k < 0x64U; ++k, ++l) x[k] = (a[l - 0x64U] - x[k - 0x25U]) & 0x3FFFFFFFULL;
	}

	v = a[i];
	*j = (i + 1U) % 0x64U;

	return v;
}


uint64_t __stdcall knuthran2002_rand(void *restrict state)
{
	register union { uint32_t d[2]; uint64_t q; } r;

	r.d[0] = (uint32_t)knuthran2002_get(state);
	r.d[0] ^= (uint32_t)(knuthran2002_get(state) << 2);
	r.d[1] = (uint32_t)knuthran2002_get(state);
	r.d[1] ^= (uint32_t)(knuthran2002_get(state) << 2);

	return r.q >> (r.d[0] % 64);

}


const uint64_t SIZEOFKR2002 = knuthran2002_state_size;


void __stdcall knuthran2002_seed(void *restrict state, uint64_t seed)
{
	uint32_t *i = state;
	int64_t *a = (int64_t*) & i[1];
	int64_t *x = &a[0x03F1], y[0xC7], s;
	register int32_t j, k, l, t;

	if (seed == 0ULL) seed = 0x4CB2FULL;
	s = (seed + 2ULL) & 0x3FFFFFFEULL;

	for (j = 0; j < 0x64; ++j)
	{
		y[j] = s;
		s <<= 1;
		if (s >= 0x40000000ULL) 
			s -= 0x3FFFFFFEULL;
	}

	y[1]++;
	s = seed & 0x3FFFFFFFULL;
	t = 0x45;

	while (t)
	{
		for (j = 0x64 - 1; j > 0; --j)
		{
			y[j + j] = y[j];
			y[j + j - 1] = 0;
		}

		for (j = 0xC6; j >= 0x64; --j)
		{
			y[j - 0x3F] = (y[j - 0x3F] - y[j]) & 0x3FFFFFFFULL;
			y[j - 0x64] = (y[j - 0x64] - y[j]) & 0x3FFFFFFFULL;
		}

		if (s & 1)
		{
			for (j = 0x64; j > 0; --j) y[j] = y[j - 1];
			y[0] = y[0x64];
			y[0x25] = (y[0x25] - y[0x64]) & 0x3FFFFFFFULL;
		}

		if (s) s >>= 1;
		else t--;
	}

	for (j = 0; j < 0x25; ++j) x[j + 0x3F] = y[j];
	for (; j < 0x64; ++j) x[j - 0x25] = y[j];

	for (j = 0; j < 10; ++j)
	{
		for (l = 0; l < 0x64; ++l) y[l] = x[l];
		for (; l < 0xC7; ++l) y[l] = (y[l - 0x64] - y[l - 0x25]) & 0x3FFFFFFFULL;
		for (k = 0; k < 0x25; ++k, ++l) x[k] = (y[l - 0x64] - y[l - 0x25U]) & 0x3FFFFFFFULL;
		for (; k < 0x64; ++k, ++l) x[k] = (y[l - 0x64] - x[k - 0x25]) & 0x3FFFFFFFULL;
	}

	*i = 0U;
}

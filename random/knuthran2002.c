// knuthran2002.c


#include "../config.h"


#define knuthran2002_maximum 0x3FFFFFFFULL
#define knuthran2002_minimum 0ULL
#define knuthran2002_state_size (sizeof(uint32_t) + (sizeof(int64_t) * 0x03F1ULL) + (sizeof(int64_t) * 0x64))


inline static uint64_t knuthran2002_get(void* state)
{
	uint32_t *j = state, i = *j, k, l;
	int64_t* a = (int64_t*)&j[1U];
	int64_t* x = &a[0x03F1ULL];
	uint64_t v;
	if (i == 0U) 
	{
		for (l = 0; l < 0x64U; ++l) a[l] = x[l];
		for (; l < 0x03F1ULL; ++l) a[l] = (a[l - 0x64U] - a[l - 0x25U]) & (0x40000000ULL - 1);
		for (k = 0; k < 0x25U; ++k, ++l) x[k] = (a[l - 0x64U] - a[l - 0x25U]) & (0x40000000ULL - 1);
		for (; k < 0x64U; ++k, ++l) x[k] = (a[l - 0x64U] - x[k - 0x25U]) & (0x40000000ULL - 1);
	}
	v = a[i];
	*j = (i + 1U) % 0x64U;
	return v;
}


inline static void knuthran2002_seed(void* state, uint64_t seed)
{
	uint32_t *i = state;
	int64_t *a = (int64_t*)&i[1U];
	int64_t *x = &a[0x03F1ULL], y[0x64U + 0x64U - 1U], s;
	register int32_t j, k, l, t;

	if (seed == 0ULL) seed = 0x4CB2FULL;
	s = (seed + 2ULL) & (0x40000000ULL - 2U);

	for (j = 0; j < 0x64; ++j)
	{
		y[j] = s;
		s <<= 1;
		if (s >= 0x40000000ULL) 
			s -= 0x40000000ULL - 2;
	}

	y[1]++;
	s = seed & (0x40000000ULL - 1);
	t = 0x46 - 1;

	while (t)
	{
		for (j = 0x64 - 1; j > 0; --j)
		{
			y[j + j] = y[j];
			y[j + j - 1] = 0;
		}
		for (j = 0x64 + 0x64 - 2; j >= 0x64; --j)
		{
			y[j - (0x64U - 0x25U)] = (y[j - (0x64U - 0x25U)] - y[j]) & (0x40000000ULL - 1);
			y[j - 0x64U] = (y[j - 0x64U] - y[j]) & (0x40000000ULL - 1);
		}
		if (s & 1)
		{
			for (j = 0x64; j > 0; --j) y[j] = y[j - 1];
			y[0U] = y[0x64U];
			y[0x25U] = (y[0x25U] - y[0x64U]) & (0x40000000ULL - 1);
		}

		if (s) s >>= 1;
		else t--;
	}

	for (j = 0; j < 0x25; ++j) x[j + 0x64 - 0x25] = y[j];
	for (; j < 0x64; ++j) x[j - 0x25] = y[j];

	for (j = 0; j < 10; ++j)
	{
		for (l = 0; l < 0x64U; ++l) y[l] = x[l];
		for (; l < 0x64 + 0x64 - 1; ++l) y[l] = (y[l - 0x64U] - y[l - 0x25U]) & (0x40000000ULL - 1);
		for (k = 0; k < 0x25U; ++k, ++l) x[k] = (y[l - 0x64U] - y[l - 0x25U]) & (0x40000000ULL - 1);
		for (; k < 0x64U; ++k, ++l) x[k] = (y[l - 0x64U] - x[k - 0x25U]) & (0x40000000ULL - 1);
	}

	*i = 0;
}

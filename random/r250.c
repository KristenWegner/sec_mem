// r250.c


#include "../config.h"


#define r250_maximum 0xFFFFFFFFULL
#define r250_minimum 0ULL
#define r250_state_size (sizeof(uint32_t) + (sizeof(uint64_t) * 0xFAU))


static uint64_t r250_get(void* state)
{
	uint32_t j, *i = state;
	uint64_t k, *x = (uint64_t*)&i[1];

	if (*i >= 0x93U) j = *i - 0x93U;
	else j = *i + 0x67U;

	k = x[*i] ^ x[j];
	x[*i] = k;

	if (i >= 0xF9U) *i = 0U;
	else *i = *i + 1U;

	return k;
}


static void r250_seed(void* state, uint64_t seed)
{
	uint32_t j, k, *i = state;
	uint64_t b, m, *x = (uint64_t*)&i[1];

	if (seed == 0ULL) seed = 1ULL;

	*i = 0U;

	for (j = 0U; j < 0xFAU; ++j)
		x[j] = (seed = ((0x10DCDULL * seed) & 0xFFFFFFFFULL));

	b = 0x80000000ULL;
	m = 0xFFFFFFFFULL;

	for (j = 0; j < 0x20U; ++j)
	{
		k = 7U * j + 3U;
		x[k] &= m;
		x[k] |= b;
		m >>= 1;
		b >>= 1;
	}
}


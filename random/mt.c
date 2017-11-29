// mt.c


#include "../config.h"


#define mt_maximum 0xFFFFFFFFULL
#define mt_minimum 0ULL
#define mt_state_size (sizeof(int32_t) + (sizeof(uint64_t) * 0x0270))


inline static uint64_t mt_get(void* state)
{
	int32_t *i = state, j;
	uint64_t *const m = (uint64_t*)&i[1];
	uint64_t k, y;

	if (*i >= 0x0270)
	{
		for (j = 0; j < 0x0270 - 0x018D; ++j)
		{
			y = (m[j] & 0x80000000UL) | (m[j + 1] & 0x7FFFFFFFUL);
			m[j] = m[j + 0x018D] ^ (y >> 1) ^ ((y & 1) ? 0X9908B0DFULL : 0ULL);
		}

		for (; j < 0x0270 - 1; ++j)
		{
			y = (m[j] & 0x80000000UL) | (m[j + 1] & 0x7FFFFFFFUL);
			m[j] = m[j + (0x018D - 0x0270)] ^ (y >> 1) ^ ((y & 1) ? 0X9908B0DFULL : 0ULL);
		}

		y = (m[0x0270 - 1] & 0x80000000UL) | (m[0] & 0x7FFFFFFFUL);
		m[0x0270 - 1] = m[0x018D - 1] ^ (y >> 1) ^ ((y & 1) ? 0X9908B0DFULL : 0ULL);

		*i = 0;
	}

	k = m[*i];
	k ^= (k >> 11);
	k ^= (k << 7) & 0x9D2C5680ULL;
	k ^= (k << 15) & 0xEFC60000ULL;
	k ^= (k >> 18);

	*i++;

	return k;
}


inline static void mt_seed(void* state, uint64_t seed)
{
	int32_t *i = state, j;
	uint64_t* m = (uint64_t*)&i[1];
	if (seed == 0ULL) seed = 0x1105ULL;
	for (j = 0; j < 0x0270; ++j)
	{
		m[j] = seed & 0xFFFF0000UL;
		seed = (((0x10DCDULL * seed) + 1ULL) & 0xFFFFFFFFULL);
		m[j] |= (seed & 0xFFFF0000UL) >> 16;
		seed = (((0x10DCDULL * seed) + 1ULL) & 0xFFFFFFFFULL);
	}
	*i = j;
}


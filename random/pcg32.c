// pcg32.c


#include "../config.h"


#define pcg32_maximum 0xFFFFFFFFULL
#define pcg32_minimum 0ULL
#define pcg32_state_size (sizeof(uint64_t) * 2U)


inline static void pcg32_seed(void* state, uint64_t seed)
{
	uint64_t* s = state;
	s[0] = 0ULL;
	s[1] = (0xDA3E39CB94B95BDBULL << 1U) | 1U;
	pcg32_get(state);
	if (seed == 0ULL) seed = 0x853C49E6748FEA9BULL;
	s[0] += seed;
	pcg32_get(state);
}


inline static uint64_t pcg32_get(void* state)
{
	uint64_t* s = state;
	uint64_t o = s[0];
	s[0] = o * 0x5851F42D4C957F2DULL + s[1];
	uint64_t x = ((o >> 0x12U) ^ o) >> 0x1BU;
	uint64_t r = o >> 0x3BU;
	return (x >> r) | (x << ((-r) & 0x1FU));
}


// lecuyer.c


#include "../config.h"


#define lecuyer_maximum 0x7FFFFF06ULL
#define lecuyer_minimum 1ULL
#define lecuyer_state_size sizeof(uint64_t)


uint32_t __stdcall lecuyer_get(void* state)
{
	register uint64_t* x = state;
	register int32_t y = *x, r = 0x0ECFL * (y / 0xCE26L);
	y = 0x9EF4L * (y % 0xCE26L) - r;
	if (y < 0L) y += 0x7FFFFF07L;
	*x = y;
	return (uint32_t)*x;
}


void __stdcall lecuyer_seed(void* state, uint64_t seed)
{
	uint64_t* x = state;
	if ((seed % 0x7FFFFF07ULL) == 0ULL) seed = 1ULL;
	*x = seed % 0x7FFFFF07ULL;
}


// ran0.c


#include "../config.h"


#define ran0_maximum 0x7FFFFFFEULL
#define ran0_minimum 1ULL
#define ran0_state_size sizeof(uint64_t)


static uint64_t ran0_get(void* state)
{
	uint64_t *x = state;
	int64_t h = *x / 0x1F31DL;
	int64_t t = 0x41A7L * (*x - h * 0x1F31DL) - h * 0x0B14L;
	if (t < 0L) *x = t + 0x7FFFFFFFL;
	else *x = t;
	return *x;
}


static void ran0_seed(void* state, uint64_t seed)
{
	uint64_t *x = state;
	if (seed == 0x075BD924ULL) seed++;
	*x = seed ^ 0x075BD924ULL;
}


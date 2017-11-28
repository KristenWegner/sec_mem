// minstd.c


#include "../config.h"


#define minstd_maximum 0x7FFFFFFEULL
#define minstd_minimum 1ULL
#define minstd_state_size sizeof(uint64_t)


inline static uint64_t minstd_get(void* state)
{
	uint64_t* x = state;
	const uint64_t y = *x;
	const int64_t h = y / 0x1F31DL, t = 0x41A7L * (y - h * 0x1F31DL) - h * 0x0B14L;
	if (t < 0L) *x = t + 0x7FFFFFFFL;
	else *x = t;
	return *x;
}


inline static void minstd_seed(void* state, uint64_t seed)
{
	uint64_t* x = state;
	if (seed == 0ULL) seed = 1ULL;
	*x = seed;
}


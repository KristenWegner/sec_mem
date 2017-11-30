// fishman2x.c


#include "../config.h"


#define fishman2x_maximum (0x7FFFFFFEUL)
#define fishman2x_minimum (0ULL)
#define fishman2x_state_size (sizeof(uint64_t) * 3)


static uint64_t fishman2x_get(void* state)
{
	uint64_t* s = state;
	int64_t r = 0x0D47ULL * (s[0] / 0xADC8ULL);
	int64_t y = 0xBC8FULL * (s[0] % 0xADC8ULL) - r;
	if (y < 0LL) y += 0x7FFFFFFFULL;
	s[0] = y;
	r = 0x0ECFULL * (s[1] / 0xCE26ULL);
	y = 0x9EF4ULL * (s[1] % 0xCE26ULL) - r;
	if (y < 0LL) y += 0x7FFFFF07ULL;
	s[1] = y;
	s[2] = (s[0] > s[1]) ? (s[0] - s[1]) : 0x7FFFFFFFULL + s[0] - s[1];
	return s[2];
}


static void fishman2x_seed(void* state, uint64_t seed)
{
	uint64_t* s = state;
	if ((seed % 0x7FFFFFFFULL) == 0ULL || (seed % 0x7FFFFF07ULL) == 0ULL) seed = 1ULL;
	s[0] = seed % 0x7FFFFFFFULL;
	s[1] = seed % 0x7FFFFF07ULL;
	s[2] = (s[0] > s[1]) ? (s[0] - s[1]) : 0x7FFFFFFFULL + s[0] - s[1];
}


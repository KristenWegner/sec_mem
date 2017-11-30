// fishman20.c


#include "../config.h"


#define fishman20_maximum (0x7FFFFFFEUL)
#define fishman20_minimum (1ULL)
#define fishman20_state_size sizeof(uint64_t)


static uint64_t fishman20_get(void* state)
{
	uint64_t* s = state;
	const uint64_t x = *s;
	const int64_t h = x / 0xADC8LL;
	const int64_t t = 0xBC8FLL * (x - h * 0xADC8LL) - h * 0x0D47LL;
	if (t < 0LL) *s = t + 0x7FFFFFFFLL;
	else *s = t;
	return *s;
}


static void fishman20_seed(void* state, uint64_t seed)
{
	uint64_t* s = state;
	if ((seed % 0x7FFFFFFFLL) == 0ULL) seed = 1ULL;
	*s = seed & 0x7FFFFFFFLL;
}


// fishman18.c


#include "../config.h"

#include "schrage.c"


#define fishman18_maximum (0x7FFFFFFEUL)
#define fishman18_minimum (1ULL)
#define fishman18_state_size sizeof(uint64_t)


inline static uint64_t fishman18_get(void* state)
{
	uint64_t* s = state;
	*s = schrage_mult(0x03B36AB7ULL, *s, 0x7FFFFFFFULL, 0xB505ULL);
	return *s;
}


inline static void fishman18_seed(void* state, uint64_t seed)
{
	uint64_t* s = state;
	if ((seed % 0x7FFFFFFFULL) == 0ULL) seed = 1ULL;
	*s = seed % 0x7FFFFFFFULL;
}


// knuthran2.c


#include "../config.h"

#include "schrage.c"


#define knuthran2_maximum 0x7FFFFFFEULL
#define knuthran2_minimum 0ULL
#define knuthran2_state_size (sizeof(uint64_t) * 2)


inline static uint64_t knuthran2_get(void *state)
{
	uint64_t* s = state;
	const unsigned long int t = s[1];
	s[1] = schrage_mult(0x1033C4D7ULL, s[1], 0x7FFFFFFFULL, 0xB505ULL) + schrage_mult(0x6D464F5AULL, s[0], 0x7FFFFFFFULL, 0xB505ULL);
	if (s[1] >= 0x7FFFFFFFULL) s[1] -= 0x7FFFFFFFULL;
	s[0] = t;
	return s[1];
}


inline static void knuthran2_seed(void* state, uint64_t seed)
{
	uint64_t* s = state;
	if ((seed % 0x7FFFFFFFULL) == 0) seed = 1ULL;
	s[0] = seed % 0x7FFFFFFFULL;
	s[1] = seed % 0x7FFFFFFFULL;
}


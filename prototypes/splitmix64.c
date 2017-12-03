
#include "../config.h"


void __stdcall splitmix64_seed(void* state, uint64_t seed)
{
	uint64_t* s = (uint64_t*)state;
	*s = seed;
}


uint64_t __stdcall splitmix64_get(void* state)
{
	uint64_t* s = (uint64_t*)state;
	*s += UINT64_C(0x9E3779B97F4A7C15);
	register uint64_t z = *s;
	z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
	return z ^ (z >> 31);
}

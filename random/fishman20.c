// fishman20.c


#include "../config.h"

#define fishman20_maximum (0x7FFFFFFEUL)
#define fishman20_minimum (1ULL)
#define fishman20_state_size sizeof(uint64_t)


inline static uint64_t fishman20_get(register void *restrict state)
{
	register uint64_t* s = state;
	const uint64_t x = *s;
	const int64_t h = x / 0xADC8LL;
	const int64_t t = 0xBC8FLL * (x - h * 0xADC8LL) - h * 0x0D47LL;
	if (t < 0LL) *s = t + 0x7FFFFFFFLL;
	else *s = t;
	return *s;
}


uint64_t __stdcall fishman20_rand(void *restrict state)
{
	register union { uint32_t d[2]; uint64_t q; } r;

	r.d[0] = (uint32_t)fishman20_get(state);
	r.d[0] ^= (uint32_t)(fishman20_get(state) << 2);
	r.d[1] = (uint32_t)fishman20_get(state);
	r.d[1] ^= (uint32_t)(fishman20_get(state) << 2);

	return r.q;
	
}


void __stdcall fishman20_seed(void *restrict state, uint64_t seed)
{
	uint64_t* s = state;
	if ((seed % 0x7FFFFFFFULL) == 0ULL) seed = 1ULL;
	*s = seed & 0x7FFFFFFFULL;
}





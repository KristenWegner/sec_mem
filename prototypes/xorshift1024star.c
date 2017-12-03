
#include <stdint.h>




uint64_t __stdcall xorshift1024star_get(void* state)
{
	register int32_t* p = state;
	register uint64_t* s = (uint64_t*)&p[1];
	const uint64_t s0 = s[*p];
	uint64_t s1 = s[*p = (*p + 1) & 15];
	s1 ^= s1 << 31;
	s[*p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
	return s[*p] * UINT64_C(0x9E3779B97F4A7C13);
}


inline static uint64_t get(void* state)
{
	register int32_t* p = state;
	register uint64_t* s = (uint64_t*)&p[1];
	const uint64_t s0 = s[*p];
	uint64_t s1 = s[*p = (*p + 1) & 15];
	s1 ^= s1 << 31;
	s[*p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
	return s[*p] * UINT64_C(0x9E3779B97F4A7C13);
}


void __stdcall xorshift1024star_seed(void* state, uint64_t seed)
{
	register int32_t* p = state;
	register uint64_t* s = (uint64_t*)&p[1];
	register uint8_t i;
	register uint64_t z;

	if (state == NULL) return;

	*p = 0;

	for (i = 0; i < 16; ++i)
	{
		z = (seed += UINT64_C(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
		s[i] = z ^ (z >> 31);
	}
}


#include "../config.h"

uint64_t s[2];

inline static uint64_t rotl(const uint64_t x, int32_t k)
{
	return (x << k) | (x >> (64 - k));
}

inline static void step(uint64_t* s)
{
	const uint64_t s0 = s[0];
	uint64_t s1 = s[1];
	s1 ^= s0;
	s[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14);
	s[1] = rotl(s1, 36);
}

inline static void xoroshiro128plus_jump(register uint64_t* s)
{
	uint64_t s0 = 0;
	uint64_t s1 = 0;
	register uint8_t b;
	
	for (b = 0; b < 64; ++b)
	{
		if (UINT64_C(0xBEAC0467EBA5FACB) & UINT64_C(1) << b)
		{ s0 ^= s[0]; s1 ^= s[1]; }
		step(s);
	}

	for (b = 0; b < 64; ++b)
	{
		if (UINT64_C(0xD86B048B86AA9922) & UINT64_C(1) << b)
		{ s0 ^= s[0]; s1 ^= s[1]; }
		step(s);
	}

	s[0] = s0;
	s[1] = s1;
}


void __stdcall xoroshiro128plus_seed(void* state, uint64_t seed)
{
	register uint64_t* s = (uint64_t*)state;
	s[0] = (seed ^ (seed >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	s[1] = (seed ^ (seed >> 27)) * UINT64_C(0x94D049BB133111EB);
	xoroshiro128plus_jump(s);
}

uint64_t __stdcall xoroshiro128plus_get(void* state)
{
	register uint64_t* s = (uint64_t*)state;

	const uint64_t s0 = s[0];
	uint64_t s1 = s[1];
	const uint64_t r = s0 + s1;

	s1 ^= s0;
	s[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14);
	s[1] = rotl(s1, 36);

	return r;
}




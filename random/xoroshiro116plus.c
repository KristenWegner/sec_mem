
#include <stdint.h>

#define UINT58MASK (uint64_t)((UINT64_C(1) << 58) - 1)

uint64_t s[2];

static inline uint64_t rotl58(const uint64_t x, int k)
{
	return (x << k) & UINT58MASK | (x >> (58 - k));
}

uint64_t next(void)
{
	uint64_t s1 = s[1];
	const uint64_t s0 = s[0];
	const uint64_t result = (s0 + s1) & UINT58MASK;
	s1 ^= s0;
	s[0] = rotl58(s0, 24) ^ s1 ^ ((s1 << 2) & UINT58MASK);
	s[1] = rotl58(s1, 35);
	return result;
}

void jump(void)
{
	static const uint64_t JUMP[] = { 0x4a11293241fcb12a, 0x0009863200f83fcd };
	uint64_t s0 = 0;
	uint64_t s1 = 0;
	for (int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
		for (int b = 0; b < 64; b++) {
			if (JUMP[i] & UINT64_C(1) << b) {
				s0 ^= s[0];
				s1 ^= s[1];
			}
			next();
		}
	s[0] = s0;
	s[1] = s1;
}


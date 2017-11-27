
#include <stdint.h>

uint64_t s[16];
int p;

uint64_t next()
{
	const uint64_t s0 = s[p];
	uint64_t s1 = s[p = (p + 1) & 15];
	s1 ^= s1 << 31; // a
	s[p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
	return s[p] * 0x9E3779B97F4A7C13;
}

void jump(void)
{
	static const uint64_t JUMP[] = { 0x84242f96eca9c41d,
		0xa3c65b8776f96855, 0x5b34a39f070b5837, 0x4489affce4f31a1e,
		0x2ffeeb0a48316f40, 0xdc2d9891fe68c022, 0x3659132bb12fea70,
		0xaac17d8efa43cab8, 0xc4cb815590989b13, 0x5ee975283d71c93b,
		0x691548c86c1bd540, 0x7910c41d10a1e6a5, 0x0b5fc64563b3e2a8,
		0x047f7684e9fc949d, 0xb99181f2d8f685ca, 0x284600e3f30e38c3
	};

	uint64_t t[16] = { 0 };
	for (int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
		for (int b = 0; b < 64; b++) {
			if (JUMP[i] & UINT64_C(1) << b)
				for (int j = 0; j < 16; j++)
					t[j] ^= s[(j + p) & 15];
			next();
		}

	for (int j = 0; j < 16; j++)
		s[(j + p) & 15] = t[j];
}


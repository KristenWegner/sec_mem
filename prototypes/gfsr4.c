// gfsr4.c


#include "../config.h"


#define gfsr4_maximum 0xFFFFFFFFULL
#define gfsr4_minimum 0ULL
#define gfsr4_state_size (sizeof(int32_t) + (sizeof(uint32_t) * 0x4000))


uint32_t __stdcall gfsr4_get(void* state)
{
	register int32_t* n = state;
	register uint32_t* a = (uint32_t*)&n[1];
	*n = ((*n) + 1) & 0x3FFF;
	return a[*n] = a[(*n + (0x3FFF + 1 - 0x01D7)) & 0x3FFF] ^ a[(*n + (0x3FFF + 1 - 0x0632)) & 0x3FFF] ^ a[(*n + (0x3FFF + 1 - 0x1B4C)) & 0x3FFF] ^ a[(*n + (0x3FFF + 1 - 0x25D9)) & 0x3FFF];
}


void __stdcall gfsr4_seed(void* state, uint64_t seed)
{
	register int32_t i, j, k, *n = state;
	uint32_t  t, b, m = 0x80000000UL, s = 0xFFFFFFFFUL;
	register uint32_t *a = (uint32_t*)&n[1];

	if (seed == 0ULL) seed = 0x1105ULL;

	for (i = 0; i <= 0x3FFF; ++i)
	{
		t = 0;
		b = m;

		for (j = 0; j < 32; ++j)
		{
			seed = ((0x10DCDUL * seed) & 0xFFFFFFFFUL);
			if (seed & m) t |= b;
			b >>= 1;
		}

		a[i] = t;
	}

	for (i = 0; i < 32; ++i)
	{
		k = 7 + i * 3;
		a[k] &= s;
		a[k] |= m;
		s >>= 1;
		m >>= 1;
	}

	*n = i;
}


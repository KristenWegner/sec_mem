// gfsr4.c


#include "../config.h"


#define gfsr4_maximum 0xFFFFFFFFULL
#define gfsr4_minimum 0ULL
#define gfsr4_state_size (sizeof(int32_t) + (sizeof(uint64_t) * 0x4000))


static uint64_t gfsr4_get(void* state)
{
	int32_t* n = state;
	uint64_t* a = (uint64_t*)&n[1];
	*n = ((*n) + 1) & 0x3FFF;
	return a[*n] = a[(*n + (0x3FFF + 1 - 0x01D7)) & 0x3FFF] ^ a[(*n + (0x3FFF + 1 - 0x0632)) & 0x3FFF] ^ a[(*n + (0x3FFF + 1 - 0x1B4C)) & 0x3FFF] ^ a[(*n + (0x3FFF + 1 - 0x25D9)) & 0x3FFF];
}


static void gfsr4_seed(void* state, uint64_t seed)
{
	int32_t i, j, k, *n = state;
	uint64_t  t, b, m = 0x80000000UL, s = 0xFFFFFFFFUL, *a = (uint64_t*)&n[1];
	if (seed == 0) seed = 0x1105;
	for (i = 0; i <= 0x3FFF; ++i)
	{
		t = 0;
		b = m;
		for (j = 0; j < 32; ++j)
		{
			seed = ((0x10DCD * seed) & 0xFFFFFFFFUL);
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


// gfsr4.c


#include <stdint.h>

/*
0xffffffffUL // Max
0 // Min
*/

// Magic numbers.

#define A 471
#define B 1586
#define C 6988
#define D 9689
#define M 16383 // 2^14-1.


#define gfsr4_state_size (sizeof(int32_t) + (sizeof(uint64_t) * 0x4000))


typedef struct
{
	int nd;
	unsigned long ra[M + 1];
}
gfsr4_state_t;

static inline uint64_t gfsr4_get(void *vstate)
{
	gfsr4_state_t *state = (gfsr4_state_t *)vstate;
	state->nd = ((state->nd) + 1) & M;
	return state->ra[(state->nd)] = state->ra[((state->nd) + (M + 1 - A)) & M] ^ state->ra[((state->nd) + (M + 1 - B)) & M] ^ state->ra[((state->nd) + (M + 1 - C)) & M] ^ state->ra[((state->nd) + (M + 1 - D)) & M];
}


static void gfsr4_seed(void *vstate, uint64_t seed)
{
	gfsr4_state_t *state = (gfsr4_state_t *)vstate;
	int i, j, k;
	uint64_t t, bit, msb = 0x80000000UL, mask = 0xFFFFFFFFUL;

	if (seed == 0) seed = 4357;

#define LCG(n) ((69069 * n) & 0xFFFFFFFFUL)

	for (i = 0; i <= M; ++i)
	{
		t = 0;
		bit = msb;

		for (j = 0; j < 32; ++j)
		{
			seed = LCG(seed);
			if (seed & msb) t |= bit;
			bit >>= 1;
		}

		state->ra[i] = t;
	}

	for (i = 0; i < 32; ++i)
	{
		k = 7 + i * 3;
		state->ra[k] &= mask;
		state->ra[k] |= msb;
		mask >>= 1;
		msb >>= 1;
	}

	state->nd = i;
}


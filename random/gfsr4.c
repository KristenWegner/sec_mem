
#include <stdlib.h>

/*
0xffffffffUL // Max
0 // Min
*/

static inline unsigned long int gfsr4_get(void *vstate);
static double gfsr4_get_double(void *vstate);
static void gfsr4_set(void *state, unsigned long int s);

// Magic numbers.

#define A 471
#define B 1586
#define C 6988
#define D 9689
#define M 16383 // 2^14-1.

typedef struct
{
	int nd;
	unsigned long ra[M + 1];
}
gfsr4_state_t;

static inline unsigned long gfsr4_get(void *vstate)
{
	gfsr4_state_t *state = (gfsr4_state_t *)vstate;

	state->nd = ((state->nd) + 1)&M;

	return state->ra[(state->nd)] =
		state->ra[((state->nd) + (M + 1 - A))&M] ^
		state->ra[((state->nd) + (M + 1 - B))&M] ^
		state->ra[((state->nd) + (M + 1 - C))&M] ^
		state->ra[((state->nd) + (M + 1 - D))&M];

}

static double gfsr4_get_double(void * vstate)
{
	return gfsr4_get(vstate) / 4294967296.0;
}

static void gfsr4_set(void *vstate, unsigned long int s)
{
	gfsr4_state_t *state = (gfsr4_state_t *)vstate;
	int i, j;
	unsigned long int msb = 0x80000000UL;
	unsigned long int mask = 0xFFFFFFFFUL;

	if (s == 0) s = 4357;

#define LCG(n) ((69069 * n) & 0xFFFFFFFFUL)

	for (i = 0; i <= M; i++)
	{
		unsigned long t = 0;
		unsigned long bit = msb;

		for (j = 0; j < 32; j++)
		{
			s = LCG(s);
			if (s & msb) t |= bit;
			bit >>= 1;
		}

		state->ra[i] = t;
	}

	for (i = 0; i < 32; ++i)
	{
		int k = 7 + i * 3;
		state->ra[k] &= mask;
		state->ra[k] |= msb;
		mask >>= 1;
		msb >>= 1;
	}

	state->nd = i;
}


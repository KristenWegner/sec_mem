
#include <stdlib.h>

/*
0xFFFFFFFFUL // Max
0 // Min
*/

static inline unsigned long int mt_get(void *vstate);
static double mt_get_double(void *vstate);
static void mt_set(void *state, unsigned long int s);

#define N 624
#define M 397

static const unsigned long UPPER_MASK = 0x80000000UL;
static const unsigned long LOWER_MASK = 0x7FFFFFFFUL;

typedef struct
{
	unsigned long mt[N];
	int mti;
}
mt_state_t;

static inline unsigned long mt_get(void *vstate)
{
	mt_state_t *state = (mt_state_t *)vstate;

	unsigned long k;
	unsigned long int *const mt = state->mt;

#define MAGIC(y) (((y)&0x1) ? 0X9908B0DFUL : 0)

	if (state->mti >= N)
	{
		int kk;

		for (kk = 0; kk < N - M; kk++)
		{
			unsigned long y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
			mt[kk] = mt[kk + M] ^ (y >> 1) ^ MAGIC(y);
		}
		for (; kk < N - 1; kk++)
		{
			unsigned long y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
			mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ MAGIC(y);
		}

		{
			unsigned long y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
			mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ MAGIC(y);
		}

		state->mti = 0;
	}

	k = mt[state->mti];
	k ^= (k >> 11);
	k ^= (k << 7) & 0x9D2C5680UL;
	k ^= (k << 15) & 0xEFC60000UL;
	k ^= (k >> 18);

	state->mti++;

	return k;
}

static double mt_get_double(void* vstate)
{
	return mt_get(vstate) / 4294967296.0;
}

static void mt_set(void *vstate, unsigned long int s)
{
	mt_state_t *state = (mt_state_t *)vstate;
	int i;

	if (s == 0) s = 4357;
	state->mt[0] = s & 0xFFFFFFFFUL;

	for (i = 1; i < N; i++)
	{
		state->mt[i] = (1812433253UL * (state->mt[i - 1] ^ (state->mt[i - 1] >> 30)) + i);
		state->mt[i] &= 0xFFFFFFFFUL;
	}

	state->mti = i;
}

static void mt_1999_set(void *vstate, unsigned long int s)
{
	mt_state_t *state = (mt_state_t *)vstate;
	int i;

	if (s == 0) s = 4357;

#define LCG(x) ((69069 * x) + 1) &0xFFFFFFFFUL

	for (i = 0; i < N; i++)
	{
		state->mt[i] = s & 0xFFFF0000UL;
		s = LCG(s);
		state->mt[i] |= (s & 0xFFFF0000UL) >> 16;
		s = LCG(s);
	}

	state->mti = i;
}

static void mt_1998_set(void *vstate, unsigned long int s)
{
	mt_state_t *state = (mt_state_t *)vstate;
	int i;

	if (s == 0) s = 4357;
	state->mt[0] = s & 0xFFFFFFFFUL;

#define LCG1998(n) ((69069 * n) & 0xFFFFFFFFUL)

	for (i = 1; i < N; i++)
		state->mt[i] = LCG1998(state->mt[i - 1]);
	state->mti = i;
}

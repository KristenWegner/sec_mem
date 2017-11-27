
#include <stdlib.h>

/*
M_BIG // Max
0 // Min
*/

static inline unsigned long int ran3_get(void *vstate);
static double ran3_get_double(void *vstate);
static void ran3_set(void *state, unsigned long int s);

#define M_BIG 1000000000
#define M_SEED 161803398

typedef struct
{
	unsigned int x;
	unsigned int y;
	unsigned long int buffer[56];
}
ran3_state_t;

static inline unsigned long int ran3_get(void *vstate)
{
	ran3_state_t *state = (ran3_state_t *)vstate;
	long int j;
	state->x++;
	if (state->x == 56) state->x = 1;
	state->y++;
	if (state->y == 56) state->y = 1;
	j = state->buffer[state->x] - state->buffer[state->y];
	if (j < 0) j += M_BIG;
	state->buffer[state->x] = j;
	return j;
}

static double ran3_get_double(void *vstate)
{
	return ran3_get(vstate) / (double)M_BIG;
}

static void ran3_set(void *vstate, unsigned long int s)
{
	ran3_state_t *state = (ran3_state_t *)vstate;
	int i, i1;
	long int j, k;
	if (s == 0) s = 1;
	j = (M_SEED - s) % M_BIG;
	if (j < 0) j += M_BIG;
	state->buffer[0] = 0;
	state->buffer[55] = j;
	k = 1;

	for (i = 1; i < 55; i++)
	{
		int n = (21 * i) % 55;
		state->buffer[n] = k;
		k = j - k;
		if (k < 0) k += M_BIG;
		j = state->buffer[n];

	}

	for (i1 = 0; i1 < 4; i1++)
	{
		for (i = 1; i < 56; i++)
		{
			long int t = state->buffer[i] - state->buffer[1 + (i + 30) % 55];
			if (t < 0) t += M_BIG;
			state->buffer[i] = t;
		}
	}

	state->x = 0;
	state->y = 31;

	return;
}


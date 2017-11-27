
#include <stdlib.h>

/*
2147483646 // Max
1 // Min
*/

static inline unsigned long int ran1_get(void *vstate);
static double ran1_get_double(void *vstate);
static void ran1_set(void *state, unsigned long int s);

static const long int m = 2147483647, a = 16807, q = 127773, r = 2836;

#define N_SHUFFLE 32
#define N_DIV (1 + 2147483646/N_SHUFFLE)

typedef struct
{
	unsigned long int x;
	unsigned long int n;
	unsigned long int shuffle[N_SHUFFLE];
}
ran1_state_t;

static inline unsigned long int ran1_get(void *vstate)
{
	ran1_state_t *state = (ran1_state_t *)vstate;
	const unsigned long int x = state->x;
	const long int h = x / q;
	const long int t = a * (x - h * q) - h * r;
	if (t < 0) state->x = t + m;
	else state->x = t;

	{
		unsigned long int j = state->n / N_DIV;
		state->n = state->shuffle[j];
		state->shuffle[j] = state->x;
	}

	return state->n;
}

static double ran1_get_double(void *vstate)
{
	float x_max = 1 - 1.2E-7F;
	float x = ran1_get(vstate) / 2147483647.0F;
	if (x > x_max) return x_max;
	return x;
}

static void ran1_set(void *vstate, unsigned long int s)
{
	ran1_state_t *state = (ran1_state_t *)vstate;
	int i;
	if (s == 0) s = 1;

	for (i = 0; i < 8; i++)
	{
		long int h = s / q;
		long int t = a * (s - h * q) - h * r;
		if (t < 0)
			t += m;
		s = t;
	}

	for (i = N_SHUFFLE - 1; i >= 0; i--)
	{
		long int h = s / q;
		long int t = a * (s - h * q) - h * r;
		if (t < 0) t += m;
		s = t;
		state->shuffle[i] = s;
	}

	state->x = s;
	state->n = s;

	return;
}


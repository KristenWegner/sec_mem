
#include <stdlib.h>

/*
0x00FFFFFF // Max
0 // Min
*/

static inline unsigned long int ranlux_get(void *vstate);
static double ranlux_get_double(void *vstate);
static void ranlux_set_lux(void *state, unsigned long int s, unsigned int luxury);
static void ranlux_set(void *state, unsigned long int s);
static void ranlux389_set(void *state, unsigned long int s);

static const unsigned long int mask_lo = 0x00FFFFFFUL;
static const unsigned long int mask_hi = ~0x00FFFFFFUL;
static const unsigned long int two24 = 16777216;

typedef struct
{
	unsigned int i;
	unsigned int j;
	unsigned int n;
	unsigned int skip;
	unsigned int carry;
	unsigned long int u[24];
}
ranlux_state_t;

static inline unsigned long int increment_state(ranlux_state_t * state)
{
	unsigned int i = state->i;
	unsigned int j = state->j;
	long int delta = state->u[j] - state->u[i] - state->carry;
	if (delta & mask_hi)
	{
		state->carry = 1;
		delta &= mask_lo;
	}
	else state->carry = 0;
	state->u[i] = delta;
	if (i == 0) i = 23;
	else i--;
	state->i = i;
	if (j == 0) j = 23;
	else j--;
	state->j = j;
	return delta;
}

static inline unsigned long int ranlux_get(void *vstate)
{
	ranlux_state_t *state = (ranlux_state_t *)vstate;
	const unsigned int skip = state->skip;
	unsigned long int r = increment_state(state);

	state->n++;

	if (state->n == 24)
	{
		unsigned int i;
		state->n = 0;
		for (i = 0; i < skip; i++)
			increment_state(state);
	}

	return r;
}

static double ranlux_get_double(void *vstate)
{
	return ranlux_get(vstate) / 16777216.0;
}

static void ranlux_set_lux(void *vstate, unsigned long int s, unsigned int luxury)
{
	ranlux_state_t *state = (ranlux_state_t *)vstate;
	int i;
	long int seed;
	if (s == 0) s = 314159265;
	seed = s;
	for (i = 0; i < 24; i++)
	{
		unsigned long int k = seed / 53668;
		seed = 40014 * (seed - k * 53668) - k * 12211;
		if (seed < 0) seed += 2147483563;
		state->u[i] = seed % two24;
	}

	state->i = 23;
	state->j = 9;
	state->n = 0;
	state->skip = luxury - 24;

	if (state->u[23] & mask_hi)
		state->carry = 1;
	else state->carry = 0;
}

static void ranlux_set(void *vstate, unsigned long int s)
{
	ranlux_set_lux(vstate, s, 223);
}

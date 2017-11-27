
#include <stdlib.h>

/*
0xFFFFFFFFUL // Max
0 // Min
*/

static inline unsigned long int r250_get(void *vstate);
static double r250_get_double(void *vstate);
static void r250_set(void *state, unsigned long int s);

typedef struct
{
	int i;
	unsigned long x[250];
}
r250_state_t;

static inline unsigned long int r250_get(void *vstate)
{
	r250_state_t *state = (r250_state_t *)vstate;
	unsigned long int k;
	int j;
	int i = state->i;

	if (i >= 147)
		j = i - 147;
	else j = i + 103;

	k = state->x[i] ^ state->x[j];
	state->x[i] = k;

	if (i >= 249)
		state->i = 0;
	else state->i = i + 1;

	return k;
}

static double r250_get_double(void *vstate)
{
	return r250_get(vstate) / 4294967296.0;
}

static void r250_set(void *vstate, unsigned long int s)
{
	r250_state_t *state = (r250_state_t *)vstate;
	int i;
	if (s == 0) s = 1;
	state->i = 0;

#define LCG(n) ((69069 * n) & 0xffffffffUL)

	for (i = 0; i < 250; i++)
	{
		s = LCG(s);
		state->x[i] = s;
	}

	{
		unsigned long int msb = 0x80000000UL;
		unsigned long int mask = 0xffffffffUL;

		for (i = 0; i < 32; i++)
		{
			int k = 7 * i + 3;
			state->x[k] &= mask;
			state->x[k] |= msb;
			mask >>= 1;
			msb >>= 1;
		}
	}
}

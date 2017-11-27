
/*
2147483646 // Max
1 // Min
*/

#include <stdlib.h>

static inline unsigned long int ran0_get(void *vstate);
static double ran0_get_double(void *vstate);
static void ran0_set(void *state, unsigned long int s);

static const long int m = 2147483647, a = 16807, q = 127773, r = 2836;
static const unsigned long int mask = 123459876;

typedef struct
{
	unsigned long int x;
}
ran0_state_t;

static inline unsigned long int ran0_get(void *vstate)
{
	ran0_state_t *state = (ran0_state_t *)vstate;
	const unsigned long int x = state->x;
	const long int h = x / q;
	const long int t = a * (x - h * q) - h * r;
	if (t < 0)
		state->x = t + m;
	else state->x = t;
	return state->x;
}

static double ran0_get_double(void *vstate)
{
	return ran0_get(vstate) / 2147483647.0;
}

static int ran0_set(void *vstate, unsigned long int s)
{
	ran0_state_t *state = (ran0_state_t *)vstate;
	if (s == mask) return -1; // Should not use seed == mask
	state->x = s ^ mask;
	return 0;
}


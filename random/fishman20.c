
#include <stdlib.h>

/*
2147483646 // Max
1 // Min
*/

static inline unsigned long int ran_get(void *vstate);
static double ran_get_double(void *vstate);
static void ran_set(void *state, unsigned long int s);

static const long int m = 2147483647, a = 48271, q = 44488, r = 3399;

typedef struct
{
	unsigned long int x;
}
ran_state_t;

static inline unsigned long int ran_get(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	const unsigned long int x = state->x;
	const long int h = x / q;
	const long int t = a * (x - h * q) - h * r;
	if (t < 0)
		state->x = t + m;
	else state->x = t;
	return state->x;
}

static double ran_get_double(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	return ran_get(state) / 2147483647.0;
}

static void ran_set(void *vstate, unsigned long int s)
{
	ran_state_t *state = (ran_state_t *)vstate;
	if ((s%m) == 0) s = 1;
	state->x = s & m;
	return;
}

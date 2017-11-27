
#include <stdlib.h>

/*
2147483646 // Max
1 // Min
*/

static inline unsigned long int minstd_get(void *vstate);
static double minstd_get_double(void *vstate);
static void minstd_set(void *state, unsigned long int s);

static const long int m = 2147483647, a = 16807, q = 127773, r = 2836;

typedef struct
{
	unsigned long int x;
}
minstd_state_t;

static inline unsigned long int minstd_get(void *vstate)
{
	minstd_state_t *state = (minstd_state_t *)vstate;
	const unsigned long int x = state->x;
	const long int h = x / q;
	const long int t = a * (x - h * q) - h * r;
	if (t < 0) state->x = t + m;
	else state->x = t;
	return state->x;
}

static double minstd_get_double(void *vstate)
{
	return minstd_get(vstate) / 2147483647.0;
}

static void minstd_set(void *vstate, unsigned long int s)
{
	minstd_state_t *state = (minstd_state_t *)vstate;
	if (s == 0) s = 1;
	state->x = s;
	return;
}


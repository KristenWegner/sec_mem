
#include <stdlib.h>

/*
2147483646 // Max
0 // Min
*/

static inline unsigned long int mrg_get(void *vstate);
static double mrg_get_double(void *vstate);
static void mrg_set(void *state, unsigned long int s);

static const long int m = 2147483647;
static const long int a1 = 107374182, q1 = 20, r1 = 7;
static const long int a5 = 104480, q5 = 20554, r5 = 1727;

typedef struct
{
	long int x1, x2, x3, x4, x5;
}
mrg_state_t;

static inline unsigned long int mrg_get(void *vstate)
{
	mrg_state_t *state = (mrg_state_t *)vstate;
	long int p1, h1, p5, h5;
	h5 = state->x5 / q5;
	p5 = a5 * (state->x5 - h5 * q5) - h5 * r5;
	if (p5 > 0) p5 -= m;
	h1 = state->x1 / q1;
	p1 = a1 * (state->x1 - h1 * q1) - h1 * r1;
	if (p1 < 0) p1 += m;
	state->x5 = state->x4;
	state->x4 = state->x3;
	state->x3 = state->x2;
	state->x2 = state->x1;
	state->x1 = p1 + p5;
	if (state->x1 < 0)
		state->x1 += m;
	return state->x1;
}

static double mrg_get_double(void *vstate)
{
	return mrg_get(vstate) / 2147483647.0;
}

static void mrg_set(void *vstate, unsigned long int s)
{
	mrg_state_t *state = (mrg_state_t *)vstate;
	if (s == 0) s = 1;

#define LCG(n) ((69069 * n) & 0xFFFFFFFFUL)

	s = LCG(s);
	state->x1 = s % m;
	s = LCG(s);
	state->x2 = s % m;
	s = LCG(s);
	state->x3 = s % m;
	s = LCG(s);
	state->x4 = s % m;
	s = LCG(s);
	state->x5 = s % m;

	mrg_get(state);
	mrg_get(state);
	mrg_get(state);
	mrg_get(state);
	mrg_get(state);
	mrg_get(state);

	return;
}


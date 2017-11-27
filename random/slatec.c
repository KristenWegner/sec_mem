
#include <stdlib.h>

/*
4194303 // Max
0 // Min
*/

static inline unsigned long int slatec_get(void *vstate);
static double slatec_get_double(void *vstate);
static void slatec_set(void *state, unsigned long int s);

typedef struct
{
	long int x0, x1;
}
slatec_state_t;

static const long P = 4194304;
static const long a1 = 1536;
static const long a0 = 1029;
static const long a1ma0 = 507;
static const long c = 1731;

static inline unsigned long int slatec_get(void *vstate)
{
	long y0, y1;
	slatec_state_t *state = (slatec_state_t *)vstate;
	y0 = a0 * state->x0;
	y1 = a1 * state->x1 + a1ma0 * (state->x0 - state->x1) + y0;
	y0 = y0 + c;
	state->x0 = y0 % 2048;
	y1 = y1 + (y0 - state->x0) / 2048;
	state->x1 = y1 % 2048;
	return state->x1 * 2048 + state->x0;
}

static double  slatec_get_double(void *vstate)
{
	return slatec_get(vstate) / 4194304.0;
}

static void slatec_set(void *vstate, unsigned long int s)
{
	slatec_state_t *state = (slatec_state_t *)vstate;
	s = s % 8;
	s *= P / 8;
	state->x0 = s % 2048;
	state->x1 = (s - state->x0) / 2048;
}

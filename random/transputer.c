
#include <stdlib.h>

/*
0xFFFFFFFF // Max
1 // Min
*/

typedef struct
{
	unsigned long int x;
}
transputer_state_t;

static unsigned long int transputer_get(void *vstate)
{
	transputer_state_t *state = (transputer_state_t *)vstate;
	state->x = (1664525 * state->x) & 0xFFFFFFFFUL;
	return state->x;
}

static double transputer_get_double(void *vstate)
{
	return transputer_get(vstate) / 4294967296.0;
}

static void transputer_set(void *vstate, unsigned long int s)
{
	transputer_state_t *state = (transputer_state_t *)vstate;
	if (s == 0) s = 1;
	state->x = s;
	return;
}

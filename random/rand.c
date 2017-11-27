
#include <stdlib.h>

/*
0x7fffffffUL // Max
0 // Min
*/

static inline unsigned long int rand_get(void *vstate);
static double rand_get_double(void *vstate);
static void rand_set(void *state, unsigned long int s);

typedef struct
{
	unsigned long int x;
}
rand_state_t;

static inline unsigned long int rand_get(void *vstate)
{
	rand_state_t *state = (rand_state_t*)vstate;
	state->x = (1103515245 * state->x + 12345) & 0x7FFFFFFFUL;
	return state->x;
}

static double rand_get_double(void *vstate)
{
	return rand_get(vstate) / 2147483648.0;
}

static void rand_set(void *vstate, unsigned long int s)
{
	rand_state_t *state = (rand_state_t *)vstate;
	state->x = s;
	return;
}


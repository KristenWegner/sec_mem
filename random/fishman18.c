
#include <stdlib.h>

#include "schrage.c"

/*
MM - 1 // Max
1 // Min
*/

#define AA 62089911UL
#define MM 0x7fffffffUL // 2 ^ 31 - 1.
#define CEIL_SQRT_MM 46341UL /// ceil(sqrt(2 ^ 31 - 1)).

static inline unsigned long int ran_get(void *vstate);
static double ran_get_double(void *vstate);
static void ran_set(void *state, unsigned long int s);

typedef struct
{
	unsigned long int x;
}
ran_state_t;

static inline unsigned long int ran_get(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	state->x = schrage_mult(AA, state->x, MM, CEIL_SQRT_MM);
	return state->x;
}

static double ran_get_double(void *vstate)
{
	ran_state_t *state = (ran_state_t*)vstate;

	return ran_get(state) / 2147483647.0;
}

static void ran_set(void *vstate, unsigned long int s)
{
	ran_state_t *state = (ran_state_t *)vstate;
	if ((s % MM) == 0) s = 1;
	state->x = s % MM;
}

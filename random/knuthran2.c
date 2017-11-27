
#include <stdlib.h>

#include "schrage.c"

/*
MM - 1L // Max
0 // Min
*/

#define AA1 271828183UL
#define AA2 1833324378UL // -314159269 % (2 ^ 31 -1).
#define MM 0x7FFFFFFFUL // 2 ^ 31 - 1.
#define CEIL_SQRT_MM 46341UL // sqrt(2 ^ 31 - 1).

static inline unsigned long int ran_get(void *vstate);
static double ran_get_double(void *vstate);
static void ran_set(void *state, unsigned long int s);

typedef struct
{
	unsigned long int x0;
	unsigned long int x1;
}
ran_state_t;

static inline unsigned long int ran_get(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	const unsigned long int xtmp = state->x1;
	state->x1 = schrage_mult(AA1, state->x1, MM, CEIL_SQRT_MM) + schrage_mult(AA2, state->x0, MM, CEIL_SQRT_MM);
	if (state->x1 >= MM) state->x1 -= MM;
	state->x0 = xtmp;
	return state->x1;
}

static double ran_get_double(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	return ran_get(state) / 2147483647.0;
}

static void ran_set(void *vstate, unsigned long int s)
{
	ran_state_t *state = (ran_state_t *)vstate;
	if ((s % MM) == 0) s = 1;
	state->x0 = s % MM;
	state->x1 = s % MM;
}


/*
MM // Max
1 // Min
*/

#define AA 1566083941UL
#define MM 0xFFFFFFFFUL

typedef struct
{
	unsigned long int x;
}
ran_state_t;

static inline unsigned long int ran_get(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	state->x = (AA * state->x) & MM;
	return state->x;
}

static double ran_get_double(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	return ran_get(state) / 4294967296.0;
}

static void ran_set(void *vstate, unsigned long int s)
{
	ran_state_t *state = (ran_state_t *)vstate;
	if (s == 0) s = 1;
	state->x = s & MM;
}

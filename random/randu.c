
/*
0x7FFFFFFF // Max
1 // Min
*/

static const long int a = 65539;

typedef struct
{
	unsigned long int x;
}
randu_state_t;

static inline unsigned long int randu_get(void *vstate)
{
	randu_state_t *state = (randu_state_t *)vstate;
	state->x = (a * state->x) & 0x7FFFFFFFUL;
	return state->x;
}

static double randu_get_double(void *vstate)
{
	return randu_get(vstate) / 2147483648.0;
}

static void randu_set(void *vstate, unsigned long int s)
{
	randu_state_t *state = (randu_state_t *)vstate;
	if (s == 0) s = 1;
	state->x = s;
	return;
}

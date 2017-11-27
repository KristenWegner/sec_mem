
/*
0xFFFFFFFF // Max
0 // Min
*/

typedef struct
{
	unsigned long int x;
}
vax_state_t;

static inline unsigned long int vax_get(void *vstate)
{
	vax_state_t *state = (vax_state_t *)vstate;
	state->x = (69069 * state->x + 1) & 0xFFFFFFFFUL;
	return state->x;
}

static double vax_get_double(void *vstate)
{
	return vax_get(vstate) / 4294967296.0;
}

static void vax_set(void *vstate, unsigned long int s)
{
	vax_state_t *state = (vax_state_t *)vstate;
	state->x = s;
}

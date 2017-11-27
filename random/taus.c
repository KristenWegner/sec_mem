
/*
0xFFFFFFFF // Max
0 // Min
*/

static inline unsigned long int taus_get(void *vstate);
static double taus_get_double(void *vstate);
static void taus_set(void *state, unsigned long int s);

typedef struct
{
	unsigned long int s1, s2, s3;
}
taus_state_t;

static inline unsigned long taus_get(void *vstate)
{
	taus_state_t *state = (taus_state_t *)vstate;

#define MASK 0xFFFFFFFFUL
#define TAUSWORTHE(s,a,b,c,d) (((s &c) <<d) &MASK) ^ ((((s <<a) &MASK)^s) >>b)

	state->s1 = TAUSWORTHE(state->s1, 13, 19, 4294967294UL, 12);
	state->s2 = TAUSWORTHE(state->s2, 2, 25, 4294967288UL, 4);
	state->s3 = TAUSWORTHE(state->s3, 3, 11, 4294967280UL, 17);

	return (state->s1 ^ state->s2 ^ state->s3);
}

static double taus_get_double(void *vstate)
{
	return taus_get(vstate) / 4294967296.0;
}

static void taus_set(void *vstate, unsigned long int s)
{
	taus_state_t *state = (taus_state_t *)vstate;

	if (s == 0) s = 1;

#define LCG(n) ((69069 * n) & 0xFFFFFFFFUL)

	state->s1 = LCG(s);
	if (state->s1 < 2) state->s1 += 2UL;
	state->s2 = LCG(state->s1);
	if (state->s2 < 8) state->s2 += 8UL;
	state->s3 = LCG(state->s2);
	if (state->s3 < 16) state->s3 += 16UL;
	taus_get(state);
	taus_get(state);
	taus_get(state);
	taus_get(state);
	taus_get(state);
	taus_get(state);
	return;
}



#include <stdlib.h>

/*
0x3FFFFFFFUL // Max
0 // Min
*/

#define BUFLEN 2009
#define KK 100
#define LL 37
#define MM (1L << 30)
#define TT 70

#define evenize(x) ((x) & (MM - 2))
#define is_odd(x) ((x) & 1)
#define mod_diff(x, y) (((x) - (y)) & (MM - 1))

static inline void ran_array(unsigned long int aa[], unsigned int n, unsigned long int ran_x[]);
static inline unsigned long int ran_get(void *vstate);
static double ran_get_double(void *vstate);
static void ran_set(void *state, unsigned long int s);

typedef struct
{
	unsigned int i;
	unsigned long int aa[BUFLEN];
	unsigned long int ran_x[KK];
}
ran_state_t;

static inline void ran_array(unsigned long int aa[], unsigned int n, unsigned long int ran_x[])
{
	unsigned int i;
	unsigned int j;

	for (j = 0; j < KK; j++)
		aa[j] = ran_x[j];

	for (; j < n; j++)
		aa[j] = mod_diff(aa[j - KK], aa[j - LL]);

	for (i = 0; i < LL; i++, j++)
		ran_x[i] = mod_diff(aa[j - KK], aa[j - LL]);

	for (; i < KK; i++, j++)
		ran_x[i] = mod_diff(aa[j - KK], ran_x[i - LL]);
}

static inline unsigned long int ran_get(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;

	unsigned int i = state->i;

	if (i == 0)
		ran_array(state->aa, BUFLEN, state->ran_x);

	state->i = (i + 1) % BUFLEN;

	return state->aa[i];
}

static double ran_get_double(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	return ran_get(state) / 1073741824.0;
}

static void ran_set(void *vstate, unsigned long int s)
{
	ran_state_t *state = (ran_state_t *)vstate;

	long x[KK + KK - 1];

	register int j;
	register int t;
	register long ss = evenize(s + 2);

	for (j = 0; j < KK; j++)
	{
		x[j] = ss;
		ss <<= 1;
		if (ss >= MM)
			ss -= MM - 2;
	}

	for (; j < KK + KK - 1; j++)
		x[j] = 0;

	x[1]++;
	ss = s & (MM - 1);
	t = TT - 1;

	while (t)
	{
		for (j = KK - 1; j > 0; j--)
			x[j + j] = x[j];
		for (j = KK + KK - 2; j > KK - LL; j -= 2)
			x[KK + KK - 1 - j] = evenize(x[j]);
		for (j = KK + KK - 2; j >= KK; j--)
		{
			if (is_odd(x[j]))
			{
				x[j - (KK - LL)] = mod_diff(x[j - (KK - LL)], x[j]);
				x[j - KK] = mod_diff(x[j - KK], x[j]);
			}
		}

		if (is_odd(ss))
		{
			for (j = KK; j > 0; j--)
				x[j] = x[j - 1];
			x[0] = x[KK];
			if (is_odd(x[KK]))
				x[LL] = mod_diff(x[LL], x[KK]);
		}

		if (ss) ss >>= 1;
		else t--;
	}

	state->i = 0;

	for (j = 0; j < LL; j++)
		state->ran_x[j + KK - LL] = x[j];
	for (; j < KK; j++)
		state->ran_x[j - LL] = x[j];

	return;
}

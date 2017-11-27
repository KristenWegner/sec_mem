
#include <stdlib.h>

/*
0x3fffffffUL // Max
0 // Min
*/

#define BUFLEN 1009
#define KK 100
#define LL 37
#define MM (1L << 30)
#define TT 70

#define is_odd(x) ((x) & 1)
#define mod_diff(x, y) (((x) - (y)) & (MM - 1))

static inline void ran_array(long int aa[], unsigned int n, long int ran_x[]);
static inline unsigned long int ran_get(void *vstate);
static double ran_get_double(void *vstate);
static void ran_set(void *state, unsigned long int s);

typedef struct
{
	unsigned int i;
	long int aa[BUFLEN];
	long int ran_x[KK];
}
ran_state_t;

static inline void ran_array(long int aa[], unsigned int n, long int ran_x[])
{
	unsigned int i, j;

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
	unsigned long int v;
	if (i == 0) ran_array(state->aa, BUFLEN, state->ran_x);
	v = state->aa[i];
	state->i = (i + 1) % KK;
	return v;
}

static double ran_get_double(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	return ran_get(state) / 1073741824.0;
}

static void ran_set(void *vstate, unsigned long int s)
{
	ran_state_t *state = (ran_state_t*)vstate;
	long x[KK + KK - 1];
	register int j;
	register int t;
	register long ss;

	if (s == 0) s = 314159;
	ss = (s + 2) & (MM - 2);

	for (j = 0; j < KK; j++)
	{
		x[j] = ss;
		ss <<= 1;
		if (ss >= MM) ss -= MM - 2;
	}

	x[1]++;
	ss = s & (MM - 1);
	t = TT - 1;

	while (t)
	{
		for (j = KK - 1; j > 0; j--)
		{
			x[j + j] = x[j];
			x[j + j - 1] = 0;
		}

		for (j = KK + KK - 2; j >= KK; j--)
		{
			x[j - (KK - LL)] = mod_diff(x[j - (KK - LL)], x[j]);
			x[j - KK] = mod_diff(x[j - KK], x[j]);
		}

		if (is_odd(ss))
		{
			for (j = KK; j > 0; j--)
				x[j] = x[j - 1];

			x[0] = x[KK];
			x[LL] = mod_diff(x[LL], x[KK]);
		}

		if (ss) ss >>= 1;
		else t--;
	}

	for (j = 0; j < LL; j++)
		state->ran_x[j + KK - LL] = x[j];

	for (; j < KK; j++)
		state->ran_x[j - LL] = x[j];

	for (j = 0; j < 10; j++)
		ran_array(x, KK + KK - 1, state->ran_x);

	state->i = 0;
}


#include <stdlib.h>

static inline unsigned long int zuf_get(void *vstate);
static double zuf_get_double(void *vstate);
static void zuf_set(void *state, unsigned long int s);
static const unsigned long int zuf_randmax = 16777216;  // 2^24.

typedef struct
{
	int n;
	unsigned long int u[607];
}
zuf_state_t;

static inline unsigned long int zuf_get(void *vstate)
{
	zuf_state_t *state = (zuf_state_t *)vstate;
	const int n = state->n;
	const int m = (n - 273 + 607) % 607;
	unsigned long int t = state->u[n] + state->u[m];

	while (t > zuf_randmax)
		t -= zuf_randmax;

	state->u[n] = t;

	if (n == 606)
	{
		state->n = 0;
	}
	else
	{
		state->n = n + 1;
	}

	return t;
}

static double zuf_get_double(void *vstate)
{
	return zuf_get(vstate) / 16777216.0;
}

static void zuf_set(void *vstate, unsigned long int s)
{
	long int kl = 9373;
	long int ij = 1802;

	/* Local variables */
	long int i, j, k, l, m;
	double x, y;
	long int ii, jj;

	zuf_state_t *state = (zuf_state_t*)vstate;

	state->n = 0;

	if (s == 0) s = 1802;

	ij = s;

	i = ij / 177 % 177 + 2;
	j = ij % 177 + 2;
	k = kl / 169 % 178 + 1;
	l = kl % 169;

	for (ii = 0; ii < 607; ++ii)
	{
		x = 0.0;
		y = 0.5;

		for (jj = 1; jj <= 24; ++jj)
		{
			m = i * j % 179 * k % 179;
			i = j;
			j = k;
			k = m;
			l = (l * 53 + 1) % 169;

			if (l * m % 64 >= 32)
				x += y;
			
			y *= 0.5;
		}
		state->u[ii] = (unsigned long int) (x * zuf_randmax);
	}
}

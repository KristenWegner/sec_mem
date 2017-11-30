// zuf.c


#include "../config.h"


#define zuf_maximum 0x1000000ULL
#define zuf_minimum 1ULL
#define zuf_state_size (sizeof(uint32_t) + (sizeof(uint64_t) * 0x025F))

typedef struct
{
	uint32_t n;
	uint64_t u[0x025F];
}
zuf_state_t;


static uint64_t zuf_get(void* state)
{
	zuf_state_t *state = (zuf_state_t *)vstate;
	uint32_t n = state->n;
	uint32_t m = (n - 0x0370) % 0x025F;
	uint64_t t = state->u[n] + state->u[m];

	while (t > 0x1000000ULL)
		t -= 0x1000000ULL;

	state->u[n] = t;

	if (n == 0x025E) state->n = 0U;
	else state->n = n + 1;

	return t;
}


static void zuf_set(void *vstate, uint64_t seed)
{
	int64_t kl = 9373, ij = 1802;
	int64_t i, j, k, l, m, ii, jj;
	double x, y;

	zuf_state_t *state = (zuf_state_t*)vstate;

	state->n = 0;

	if (seed == 0) seed = 1802;

	ij = seed;

	i = ij / 177 % 177 + 2;
	j = ij % 177 + 2;
	k = kl / 169 % 178 + 1;
	l = kl % 169;

	for (ii = 0; ii < 0x025F; ++ii)
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
		state->u[ii] = (uint64_t) (x * 0x1000000ULL);
	}
}

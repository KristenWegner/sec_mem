
/*
2147483646 // Max
0 // Min
*/

static const unsigned long int MDIG = 32; /* Machine digits in int */
static const unsigned long int m1 = 2147483647; /* 2^(MDIG-1) - 1 */
static const unsigned long int m2 = 65536; /* 2^(MDIG/2) */

typedef struct
{
	int i, j;
	unsigned long m[17];
}
uni32_state_t;

static inline unsigned long uni32_get(void *vstate)
{
	uni32_state_t *state = (uni32_state_t *)vstate;
	const long int i = state->i;
	const long int j = state->j;
	long int k = state->m[i] - state->m[j];
	if (k < 0) k += m1;
	state->m[j] = k;
	if (i == 0) state->i = 16;
	else (state->i)--;
	if (j == 0) state->j = 16;
	else (state->j)--;
	return k;
}

static double uni32_get_double(void *vstate)
{
	return uni32_get(vstate) / 2147483647.0;
}

static void uni32_set(void *vstate, unsigned long int s)
{
	long int seed, k0, k1, j0, j1;
	int i;
	uni32_state_t *state = (uni32_state_t *)vstate;
	seed = (s < m1 ? s : m1);
	seed -= (seed % 2 == 0 ? 1 : 0);
	k0 = 9069 % m2;
	k1 = 9069 / m2;
	j0 = seed % m2;
	j1 = seed / m2;
	for (i = 0; i < 17; i++)
	{
		seed = j0 * k0;
		j1 = (seed / m2 + j0 * k1 + j1 * k0) % (m2 / 2);
		j0 = seed % m2;
		state->m[i] = j0 + m2 * j1;
	}
	state->i = 4;
	state->j = 16;
}


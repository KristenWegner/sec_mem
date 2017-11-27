
/*
32766 // Max
0 // Min
*/

static const unsigned int MDIG = 16;    /* Machine digits in int */
static const unsigned int m1 = 32767;   /* 2^(MDIG-1) - 1 */
static const unsigned int m2 = 256;     /* 2^(MDIG/2) */

typedef struct
{
	int i, j;
	unsigned long m[17];
}
uni_state_t;

static inline unsigned long uni_get(void *vstate)
{
	uni_state_t *state = (uni_state_t *)vstate;
	const int i = state->i;
	const int j = state->j;
	long k = state->m[i] - state->m[j];
	if (k < 0) k += m1;
	state->m[j] = k;
	if (i == 0) state->i = 16;
	else (state->i)--;
	if (j == 0) state->j = 16;
	else (state->j)--;
	return k;
}

static double uni_get_double(void *vstate)
{
	return uni_get(vstate) / 32767.0;
}

static void uni_set(void *vstate, unsigned long int s)
{
	unsigned int i, seed, k0, k1, j0, j1;
	uni_state_t *state = (uni_state_t *)vstate;
	s = 2 * s + 1;
	seed = (s < m1 ? s : m1);
	k0 = 9069 % m2;
	k1 = 9069 / m2;
	j0 = seed % m2;
	j1 = seed / m2;
	for (i = 0; i < 17; ++i)
	{
		seed = j0 * k0;
		j1 = (seed / m2 + j0 * k1 + j1 * k0) % (m2 / 2);
		j0 = seed % m2;
		state->m[i] = j0 + m2 * j1;
	}
	state->i = 4;
	state->j = 16;
}

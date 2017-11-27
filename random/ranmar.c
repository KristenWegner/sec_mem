
#include <stdlib.h>

/*
0x00FFFFFF // Max
0 // Min
*/

static inline unsigned long int ranmar_get(void *vstate);
static double ranmar_get_double(void *vstate);
static void ranmar_set(void *state, unsigned long int s);

static const unsigned long int two24 = 16777216;

typedef struct
{
	unsigned int i;
	unsigned int j;
	long int carry;
	unsigned long int u[97];
}
ranmar_state_t;

static inline unsigned long int ranmar_get(void *vstate)
{
	ranmar_state_t *state = (ranmar_state_t *)vstate;
	unsigned int i = state->i;
	unsigned int j = state->j;
	long int carry = state->carry;
	long int delta = state->u[i] - state->u[j];
	if (delta < 0) delta += two24;
	state->u[i] = delta;
	if (i == 0) i = 96;
	else i--;
	state->i = i;
	if (j == 0) j = 96;
	else j--;
	state->j = j;
	carry += -7654321;
	if (carry < 0) carry += two24 - 3;
	state->carry = carry;
	delta += -carry;
	if (delta < 0) delta += two24;
	return delta;
}

static double ranmar_get_double(void *vstate)
{
	return ranmar_get(vstate) / 16777216.0;
}

static void ranmar_set(void *vstate, unsigned long int s)
{
	ranmar_state_t *state = (ranmar_state_t *)vstate;
	unsigned long int ij = s / 30082;
	unsigned long int kl = s % 30082;
	int i = (ij / 177) % 177 + 2;
	int j = (ij % 177) + 2;
	int k = (kl / 169) % 178 + 1;
	int l = (kl % 169);
	int a, b;
	for (a = 0; a < 97; a++)
	{
		unsigned long int sum = 0;
		unsigned long int t = two24;
		for (b = 0; b < 24; b++)
		{
			unsigned long int m = (((i * j) % 179) * k) % 179;
			i = j;
			j = k;
			k = m;
			l = (53 * l + 1) % 169;
			t >>= 1;
			if ((l * m) % 64 >= 32) sum += t;
		}
		state->u[a] = sum;
	}
	state->i = 96;
	state->j = 32;
	state->carry = 362436;
}


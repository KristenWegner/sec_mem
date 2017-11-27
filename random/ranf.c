
#include <math.h>
#include <stdlib.h>

/*
0xFFFFFFFF // Max
0 // Min
*/

static inline void ranf_advance(void *vstate);
static unsigned long int ranf_get(void *vstate);
static double ranf_get_double(void *vstate);
static void ranf_set(void *state, unsigned long int s);

static const unsigned short int a0 = 0xB175;
static const unsigned short int a1 = 0xA2E7;
static const unsigned short int a2 = 0x2875;

typedef struct
{
	unsigned short int x0, x1, x2;
}
ranf_state_t;

static inline void ranf_advance(void *vstate)
{
	ranf_state_t *state = (ranf_state_t *)vstate;

	const unsigned long int x0 = (unsigned long int) state->x0;
	const unsigned long int x1 = (unsigned long int) state->x1;
	const unsigned long int x2 = (unsigned long int) state->x2;

	unsigned long int r;

	r = a0 * x0;
	state->x0 = (r & 0xFFFF);
	r >>= 16;
	r += a0 * x1 + a1 * x0;
	state->x1 = (r & 0xFFFF);
	r >>= 16;
	r += a0 * x2 + a1 * x1 + a2 * x0;
	state->x2 = (r & 0xFFFF);
}

static unsigned long int  ranf_get(void *vstate)
{
	unsigned long int x1, x2;
	ranf_state_t *state = (ranf_state_t *)vstate;
	ranf_advance(state);
	x1 = (unsigned long int) state->x1;
	x2 = (unsigned long int) state->x2;
	return (x2 << 16) + x1;
}

static double ranf_get_double(void * vstate)
{
	ranf_state_t *state = (ranf_state_t *)vstate;

	ranf_advance(state);

	return (ldexp((double)state->x2, -16)
		+ ldexp((double)state->x1, -32)
		+ ldexp((double)state->x0, -48));
}

static void ranf_set(void *vstate, unsigned long int s)
{
	ranf_state_t *state = (ranf_state_t *)vstate;

	unsigned short int x0, x1, x2;
	unsigned long int r;

	unsigned long int b0 = 0xD6DD;
	unsigned long int b1 = 0xB894;
	unsigned long int b2 = 0x5CEE;

	if (s == 0)
	{
		x0 = 0x9CD1;
		x1 = 0x53FC;
		x2 = 0x9482;
	}
	else
	{
		x0 = (s | 1) & 0xFFFF;
		x1 = s >> 16 & 0xFFFF;
		x2 = 0;
	}

	r = b0 * x0;
	state->x0 = (r & 0xFFFF);
	r >>= 16;
	r += b0 * x1 + b1 * x0;
	state->x1 = (r & 0xFFFF);
	r >>= 16;
	r += b0 * x2 + b1 * x1 + b2 * x0;
	state->x2 = (r & 0xFFFF);

	return;
}

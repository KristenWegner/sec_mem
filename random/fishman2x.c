
#include <stdlib.h>

// Fishman
#define AAA_F 48271UL
#define MMM_F 0x7fffffffUL // 2 ^ 31 - 1.
#define QQQ_F 44488UL
#define RRR_F 3399UL

// L'Ecuyer
#define AAA_L 40692UL
#define MMM_L 0x7fffff07UL // 2 ^ 31 - 249.
#define QQQ_L 52774UL
#define RRR_L 3791UL

/*
MMM_F - 1 // Max
0 // Min
*/

static inline unsigned long int ran_get(void *vstate);
static double ran_get_double(void *vstate);
static void ran_set(void *state, unsigned long int s);

typedef struct
{
	unsigned long int x;
	unsigned long int y;
	unsigned long int z;
}
ran_state_t;

static inline unsigned long int ran_get(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;

	long int y, r;

	r = RRR_F * (state->x / QQQ_F);
	y = AAA_F * (state->x % QQQ_F) - r;
	if (y < 0)
		y += MMM_F;
	state->x = y;

	r = RRR_L * (state->y / QQQ_L);
	y = AAA_L * (state->y % QQQ_L) - r;
	if (y < 0)
		y += MMM_L;
	state->y = y;

	state->z = (state->x > state->y) ? (state->x - state->y) :
		MMM_F + state->x - state->y;

	return state->z;
}

static double ran_get_double(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;

	return ran_get(state) / 2147483647.0;
}

static void ran_set(void *vstate, unsigned long int s)
{
	ran_state_t *state = (ran_state_t *)vstate;

	if ((s % MMM_F) == 0 || (s % MMM_L) == 0) s = 1;

	state->x = s % MMM_F;
	state->y = s % MMM_L;
	state->z = (state->x > state->y) ? (state->x - state->y) : MMM_F + state->x - state->y;

	return;
}

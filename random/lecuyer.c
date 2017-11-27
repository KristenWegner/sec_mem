
#include <stdlib.h>

/*
MMM-1 // Max
1 // Min
*/

#define AAA 40692
#define MMM 2147483399UL
#define QQQ 52774
#define RRR 3791

static inline unsigned long int ran_get(void *vstate);
static double ran_get_double(void *vstate);
static void ran_set(void *state, unsigned long int s);

typedef struct
{
	unsigned long int x;
}
ran_state_t;

static inline unsigned long int ran_get(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	long int y = state->x;
	long int r = RRR * (y / QQQ);
	y = AAA * (y % QQQ) - r;
	if (y < 0) y += MMM;
	state->x = y;
	return state->x;
}

static double ran_get_double(void *vstate)
{
	ran_state_t *state = (ran_state_t *)vstate;
	return ran_get(state) / 2147483399.0;
}

static void ran_set(void *vstate, unsigned long int s)
{
	ran_state_t *state = (ran_state_t *)vstate;
	if ((s % MMM) == 0) s = 1;
	state->x = s % MMM;
	return;
}


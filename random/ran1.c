// ran1.c


#include "../config.h"


#define ran1_maximum 0x7FFFFFFEULL
#define ran1_minimum 1ULL
#define ran1_state_size (sizeof(uint64_t) * 0x22)


static const int64_t m = 2147483647, a = 16807, q = 127773, r = 2836;

typedef struct
{
	uint64_t x;
	uint64_t n;
	uint64_t shuffle[0x20];
}
ran1_state_t;

static inline uint64_t ran1_get(void* state)
{
	ran1_state_t *state = (ran1_state_t *)vstate;
	uint64_t x = state->x;
	const int64_t h = x / q;
	const int64_t t = a * (x - h * q) - h * r;
	if (t < 0) state->x = t + m;
	else state->x = t;

	{
		uint64_t j = state->n / 0x3FFFFFF;
		state->n = state->shuffle[j];
		state->shuffle[j] = state->x;
	}

	return state->n;
}

static void ran1_set(void *vstate, uint64_t s)
{
	ran1_state_t *state = (ran1_state_t *)vstate;
	int32_t i;

	if (s == 0ULL) s = 1ULL;

	for (i = 0; i < 8; ++i)
	{
		int64_t h = s / q;
		int64_t t = a * (s - h * q) - h * r;
		if (t < 0) t += m;
		s = t;
	}

	for (i = 0x1F; i >= 0; i--)
	{
		int64_t h = s / q;
		int64_t t = a * (s - h * q) - h * r;
		if (t < 0) t += m;
		s = t;
		state->shuffle[i] = s;
	}

	state->x = s;
	state->n = s;

	return;
}


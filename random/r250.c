// r250.c


#include "../config.h"


#define pcg32_maximum 0xFFFFFFFFULL
#define pcg32_minimum 0ULL
#define pcg32_state_size (sizeof(int32_t) + (sizeof(uint64_t) * 0xFAU))


typedef struct
{
	int i;
	unsigned long x[250];
}
r250_state_t;

static inline uint64_t r250_get(void *vstate)
{
	r250_state_t *state = (r250_state_t *)vstate;
	uint64_t k;
	int j, i = state->i;

	if (i >= 147) j = i - 147;
	else j = i + 103;

	k = state->x[i] ^ state->x[j];
	state->x[i] = k;

	if (i >= 0xF9U) state->i = 0U;
	else state->i = i + 1U;

	return k;
}

static void r250_seed(void *vstate, uint64_t seed)
{
	r250_state_t *state = (r250_state_t *)vstate;
	uint32_t i, k;
	uint64_t msb, mask;

	if (seed == 0ULL) seed = 1ULL;
	state->i = 0;

	for (i = 0; i < 0xFAU; ++i)
		state->x[i] = (seed = ((0x10DCDULL * seed) & 0xFFFFFFFFULL));

	msb = 0x80000000ULL;
	mask = 0xFFFFFFFFULL;

	for (i = 0; i < 0x20U; ++i)
	{
		k = 7 * i + 3;
		state->x[k] &= mask;
		state->x[k] |= msb;
		mask >>= 1;
		msb >>= 1;
	}
}

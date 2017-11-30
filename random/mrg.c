// mrg.c


#include "../config.h"


#define mrg_maximum 0x7FFFFFFEULL
#define mrg_minimum 0ULL
#define mrg_state_size (sizeof(int64_t) * 5)


static uint64_t mrg_get(void* state)
{
	int64_t* x = state;
	int64_t h5 = x[4] / 0x504AL;
	int64_t p5 = 0x19820L * (x[4] - h5 * 0x504AL) - h5 * 0x06BFL;
	if (p5 > 0) p5 -= 0x7FFFFFFFL;
	int64_t h1 = x[0] / 0x14L;
	int64_t p1 = 0x6666666L * (x[0] - h1 * 0x14L) - h1 * 0x07L;
	if (p1 < 0) p1 += 0x7FFFFFFFL;
	x[4] = x[3];
	x[3] = x[2];
	x[2] = x[1];
	x[1] = x[0];
	x[0] = p1 + p5;
	if (x[0] < 0L) x[0] += 0x7FFFFFFFL;
	return (uint64_t)x[0];
}


static void mrg_seed(void* state, uint64_t seed)
{
	int64_t* x = state;
	if (seed == 0ULL) seed = 1ULL;
	seed = ((0x10DCD * seed) & 0xFFFFFFFFULL);
	x[0] = seed % 0x7FFFFFFFL;
	seed = ((0x10DCD * seed) & 0xFFFFFFFFULL);
	x[1] = seed % 0x7FFFFFFFL;
	seed = ((0x10DCD * seed) & 0xFFFFFFFFULL);
	x[2] = seed % 0x7FFFFFFFL;
	seed = ((0x10DCD * seed) & 0xFFFFFFFFULL);
	x[3] = seed % 0x7FFFFFFFL;
	seed = ((0x10DCD * seed) & 0xFFFFFFFFULL);
	x[4] = seed % 0x7FFFFFFFL;
	mrg_get(state);
	mrg_get(state);
	mrg_get(state);
	mrg_get(state);
	mrg_get(state);
	mrg_get(state);
}


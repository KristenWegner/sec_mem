// cmrg.c


#include "../config.h"


#define cmrg_maximum (0x7FFFFFFEUL)
#define cmrg_minimum (0ULL)
#define cmrg_state_size (sizeof(int64_t) * 6)


static uint64_t cmrg_get(void* state)
{
	int64_t h1, h2, h3, p1, p2, p3;
	int64_t* st = state;
	h3 = st[2] / 0x2DC2L;
	p3 = -183326L * (st[2] - h3 * 0x2DC2L) - h3 * 0x0B43L;
	h2 = st[1] / 0x8481L;
	p2 = 0xF74CL * (st[1] - h2 * 0x8481L) - h2 * 0x32B3L;
	if (p3 < 0L) p3 += 0x7FFFFFFFL;
	if (p2 < 0L) p2 += 0x7FFFFFFFL;
	st[2] = st[1];
	st[1] = st[0];
	st[0] = p2 - p3;
	if (st[0] < 0L) st[0] += 0x7FFFFFFFL;
	h3 = st[5] / 0x0F88L;
	p3 = -539608L * (st[5] - h3 * 0x0F88L) - h3 * 0x0817L;
	h1 = st[3] / 0x6157;
	p1 = 0x15052L * (st[3] - h1 * 0x6157L) - h1 * 0x1CF9L;
	if (p3 < 0L) p3 += 0x7FE17AD7L;
	if (p1 < 0L) p1 += 0x7FE17AD7L;
	st[5] = st[4];
	st[4] = st[3];
	st[3] = p1 - p3;
	if (st[3] < 0) st[3] += 0x7FE17AD7L;
	if (st[0] < st[3]) return (st[0] - st[3] + 0x7FFFFFFFL);
	return (uint64_t)(st[0] - st[3]);
}


static void cmrg_seed(void* state, uint64_t seed)
{
	int64_t* st = state;
	if (seed == 0ULL) seed = 1ULL;
	seed = ((0x10DCDULL * seed) & 0xFFFFFFFFULL);
	st[0] = seed % 0x7FFFFFFFULL;
	seed = ((0x10DCDULL * seed) & 0xFFFFFFFFULL);
	st[1] = seed % 0x7FFFFFFFULL;
	seed = ((0x10DCDULL * seed) & 0xFFFFFFFFULL);
	st[2] = seed % 0x7FFFFFFFULL;
	seed = ((0x10DCDULL * seed) & 0xFFFFFFFFULL);
	st[3] = seed % 0x7FE17AD7ULL;
	seed = ((0x10DCDULL * seed) & 0xFFFFFFFFULL);
	st[4] = seed % 0x7FE17AD7ULL;
	seed = ((0x10DCDULL * seed) & 0xFFFFFFFFULL);
	st[5] = seed % 0x7FE17AD7ULL;
	cmrg_get(state);
	cmrg_get(state);
	cmrg_get(state);
	cmrg_get(state);
	cmrg_get(state);
	cmrg_get(state);
	cmrg_get(state);
}


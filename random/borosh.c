// borosh.c


#include <stdint.h>


#define borosh_maximum (0xFFFFFFFFUL)
#define borosh_minimum (1ULL)
#define borosh_state_size sizeof(uint64_t)


static inline uint64_t borosh_get(void* state)
{
	uint64_t* x = state;
	*x = (0x6C078965ULL * *x) & 0xFFFFFFFFULL;
	return *x;
}


static void borosh_seed(void* state, uint64_t seed)
{
	uint64_t* x = state;
	if (seed == 0ULL) seed = 1ULL;
	*x = seed & 0xFFFFFFFFULL;
}


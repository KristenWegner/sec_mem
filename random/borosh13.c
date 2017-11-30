// borosh13.c


#include "../config.h"


#define borosh13_maximum (0xFFFFFFFFUL)
#define borosh13_minimum (1ULL)
#define borosh13_state_size sizeof(uint64_t)


static uint64_t borosh13_get(void* state)
{
	uint64_t* x = state;
	*x = (0x6C078965ULL * *x) & 0xFFFFFFFFULL;
	return *x;
}


static void borosh13_seed(void* state, uint64_t seed)
{
	uint64_t* x = state;
	if (seed == 0ULL) seed = 1ULL;
	*x = seed & 0xFFFFFFFFULL;
}


// coveyou.c


#include "../config.h"


#define coveyou_maximum (0x7FFFFFFDULL)
#define coveyou_minimum (2ULL)
#define coveyou_state_size sizeof(uint64_t)


inline static uint64_t coveyou_get(void* state)
{
	uint64_t* x = state;
	*x = (*x * (*x + 1ULL)) & 0xFFFFFFFFULL;
	return *x;
}

inline static void coveyou_seed(void* state, uint64_t seed)
{
	uint64_t* x = state;
	uint64_t d = ((seed % 4ULL) - 2ULL) % 0xFFFFFFFFULL;
	if (d) *x = (seed - d) & 0xFFFFFFFFULL;
	else *x = seed & 0xFFFFFFFFULL;
}


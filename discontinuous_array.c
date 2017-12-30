// discontinuous_array.c


#include "config.h"
#include "discontinuous_array.h"


extern uint64_t callconv sm_master_rand();


inline static uint64_t rnext(void *restrict s)
{
	register uint32_t *p = s;
	register uint64_t *t = (uint64_t*)&p[1], s0 = t[*p];
	uint64_t s1 = t[*p = (*p + 1) & 15];
	s1 ^= s1 << 31;
	t[*p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
	return t[*p] * UINT64_C(0x9E3779B97F4A7C13);
}


inline static void rseed(void *state, uint64_t seed)
{
	register uint32_t i, *p = state; *p = 0;
	register uint64_t z, *s = (uint64_t*)&p[1];
	for (i = 0; i < 0x20; ++i) 
	{
		z = (seed += UINT64_C(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
		s[i] = z ^ (z >> 31);
	}
}


bool sm_discontinuous_array_create(sm_discontinuous_array** object, size_t spacing, size_t element, size_t count)
{
	if (!object) return false;
	*object = NULL;

	sm_discontinuous_array* result = malloc(sizeof(sm_discontinuous_array));
	if (!result) return false;

	register uint8_t* t = (uint8_t*)result;
	register size_t i, n = sizeof(sm_discontinuous_array);
	while (n-- > 0U) *t++ = 0;

	if (!sm_mutex_create(&result->mutex))
	{
		free(result);
		return false;
	}

	sm_mutex_lock(&result->mutex);

	result->seed = sm_master_rand();
	result->spacing = spacing;
	result->element = element;
	result->count = count;

	size_t total = ((element + spacing) * (count + 2));
	result->size = (size_t)(((total + 1.0 / 16.0) * 8) / 8.0);

	result->data = (uint8_t*)malloc(result->size);

	if (!result->data)
	{
		sm_mutex_unlock(&result->mutex);
		sm_mutex_destroy(&result->mutex);
		free(result);
		return false;
	}

	for (i = 0; i < result->size; ++i)
		result->data[i] = (uint8_t)sm_master_rand();

	result->index = 0;
	result->pointer = NULL;

	sm_mutex_unlock(&result->mutex);

	*object = result;

	return true;
}

void* sm_discontinuous_array_get(sm_discontinuous_array* object, uint64_t index)
{
	if (!object) return NULL;

	// TODO

	return NULL;
}


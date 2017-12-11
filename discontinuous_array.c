// discontinuous_array.c


#include "config.h"
#include "discontinuous_array.h"



inline static uint8_t fold64to8(uint64_t value)
{
	union { uint8_t b[sizeof(uint64_t)]; uint64_t u; } v = { .u = value };
	uint8_t r = v.b[0];
	for (uint8_t i = 1; i < sizeof(v.b); ++i) r ^= v.b[i];
	return (!r) ? 1U : r;
}


// XorShift1024* 64 next.
inline static uint64_t xorshift1024star_next(void *state)
{
	register int32_t *p = state;
	register uint64_t *s = (uint64_t*)&p[1];
	const uint64_t s0 = s[*p];
	uint64_t s1 = s[*p = (*p + 1) & 15];
	s1 ^= s1 << 31;
	s[*p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
	return s[*p] * UINT64_C(0x9E3779B97F4A7C13);
}


// XorShift1024* 64 seed.
inline static void xorshift1024star_seed(void *state, uint64_t seed)
{
	register int32_t *p = state;
	register uint64_t *s = (uint64_t*)&p[1];
	register uint8_t i;
	register uint64_t z;

	*p = 0;

	for (i = 0; i < 16; ++i) 
	{
		z = (seed += UINT64_C(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
		s[i] = z ^ (z >> 31);
	}
}


bool sec_discontinuous_array_create(sec_discontinuous_array** object, size_t spacing, size_t element, size_t count, uint64_t seed)
{
	if (!object) return false;
	*object = NULL;

	sec_discontinuous_array* result = malloc(sizeof(sec_discontinuous_array));

	if (!result) return false;

	register uint8_t* t = (uint8_t*)result;
	register size_t n = sizeof(sec_discontinuous_array);
	while (n-- > 0U) *t++ = 0x0U;

	if (!mutex_create(&result->mutex))
	{
		free(result);
		return false;
	}

	mutex_lock(&result->mutex);

	result->seed = seed;
	result->spacing = spacing;
	result->element = element;
	result->count = count;

	size_t total = ((element + spacing) * (count + 2));
	total = (size_t)(((total + 1.0 / 16.0) * 8) / 8.0);

	result->data = (uint8_t*)malloc(total);

	if (!result->data)
	{
		mutex_unlock(&result->mutex);
		mutex_destroy(&result->mutex);
		free(result);
		return false;
	}

	mutex_unlock(&result->mutex);

	*object = result;

	return true;
}
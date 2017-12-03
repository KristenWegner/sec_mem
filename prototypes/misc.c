// misc.c


#include "config.h"


// Performs p = p ^ f(s), for the given count of bytes. Returns p.
void* __stdcall sec_memxor_gen(void *restrict p, register size_t n, uint64_t(*f)(void*), void* s)
{
	register uint8_t* d = (uint8_t*)p;

	for (; n > 0U; --n)
		*d++ ^= (uint8_t)f(s);

	return p;
}


inline static int32_t sec_memcmp(const void *restrict p, const void *restrict q, register size_t n)
{
	register const uint8_t* l = (const uint8_t*)p;
	register const uint8_t* r = (const uint8_t*)q;

	while (n-- > 0U)
		if (*l++ != *r++)
			return (l[-1] < r[-1]) ? -1 : 1;

	return 0;
}


// Returns the first occurrence of q of length n in p of length m. If not found, or m less than n, returns null. 
// Returns p if n is zero.
void* __stdcall sec_memmem(const void *restrict p, size_t m, const void *restrict q, register size_t n)
{
	register const uint8_t* b;
	register const uint8_t* const l = (((const uint8_t*)p) + m - n);

	if (n == 0U) return (void*)p;
	if (m < n) return NULL;

	for (b = (const uint8_t*)p; b <= l; ++b)
		if (b[0] == ((const uint8_t*)q)[0] && !sec_memcmp((const void*)&b[1], (const void*)((const uint8_t*)q + 1), n - 1))
			return (void*)b;

	return NULL;
}


// Copies n bytes from memory region q to region p. Returns p.
void* __stdcall sec_memcpy(void *restrict p, const void *restrict q, size_t n)
{
	register const uint8_t* s;
	register uint8_t* d;

	if (p < q)
	{
		s = (const uint8_t*)q;
		d = (uint8_t*)p;
		while (n--) *d++ = *s++;
	}
	else
	{
		s = (const uint8_t*)q + (n - 1);
		d = (uint8_t*)p + (n - 1);
		while (n--) *d-- = *s--;
	}

	return p;
}


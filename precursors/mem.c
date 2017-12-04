// mem.c - Memory-related methods.


#include "../config.h"


#define EXPORTED // Just to identify exported functions.


#if defined(SEC_OS_WINDOWS)
#define CALLCONV __stdcall // Do not emit extra prologue instructions.
#elif defined(SEC_OS_LINUX)
#define CALLCONV __attribute__((stdcall))
#else
#define CALLCONV
#endif


// Performs p[i] = p[i] ^ f(s), for the given count of bytes (n). Returns p.
EXPORTED void* CALLCONV MEMXORRG(void *restrict p, register size_t n, uint64_t(*f)(void*), void* s)
{
	register uint8_t* d = (uint8_t*)p;

	while (n-- > 0U)
		*d++ ^= (uint8_t)f(s);

	return p;
}


// Local memcmp for MEMMEMFN.
inline static int32_t MEMCMP_LOCAL(const void *restrict p, const void *restrict q, register size_t n)
{
	register const uint8_t* l = (const uint8_t*)p;
	register const uint8_t* r = (const uint8_t*)q;

	while (n-- > 0U)
		if (*l++ != *r++)
			return (l[-1] < r[-1]) ? -1 : 1;

	return 0;
}


// Same as memmem. Returns the first occurrence of q of length n in p of length m. If not found, or m less than n, returns null. 
// Returns p if n is zero.
EXPORTED void* CALLCONV MEMMEMFN(const void *restrict p, size_t m, const void *restrict q, register size_t n)
{
	register const uint8_t* b;
	register const uint8_t* const l = (((const uint8_t*)p) + m - n);

	if (n == 0U) return (void*)p;
	if (m < n) return NULL;

	for (b = (const uint8_t*)p; b <= l; ++b)
		if (b[0] == ((const uint8_t*)q)[0] && !MEMCMP_LOCAL((const void*)&b[1], (const void*)((const uint8_t*)q + 1), n - 1))
			return (void*)b;

	return NULL;
}


// Same as memcpy. Copies n bytes from memory region q to region p. Returns p.
EXPORTED void* CALLCONV MEMCPYFN(void *restrict p, const void *restrict q, size_t n)
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


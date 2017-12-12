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

// Performs an XOR swap: p[i] = (p[i] ^ f1(s1)) ^ f2(s2), for the given count of bytes (n). Returns p.
EXPORTED void* CALLCONV MEMXOSWP(void *restrict p, register size_t n, uint64_t(*f1)(void*), void* s1, uint64_t(*f2)(void*), void* s2)
{
	register uint8_t* d = (uint8_t*)p;

	while (n-- > 0U)
	{
		*d ^= (uint8_t)f1(s1);
		*d++ ^= (uint8_t)f2(s2);
	}

	return p;
}


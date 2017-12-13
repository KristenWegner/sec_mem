// mem.c - Memory-related methods.


#include "../config.h"


// Performs p[i] = p[i] ^ f(s), for the given count of bytes (n). Returns p.
exported void* callconv sm_mem_xor_func(void *restrict p, register size_t n, uint64_t(*f)(void*), void* s)
{
	register uint8_t* d = (uint8_t*)p;
	while (n-- > 0) *d++ ^= (uint8_t)f(s);
	return p;
}

// Performs an XOR swap: p[i] = (p[i] ^ f1(s1)) ^ f2(s2), for the given count of bytes (n). Returns p.
exported void* callconv sm_mem_xor_swap(void *restrict p, register size_t n, uint64_t(*f1)(void*), void* s1, uint64_t(*f2)(void*), void* s2)
{
	register uint8_t* d = (uint8_t*)p;

	while (n-- > 0)
	{
		*d ^= (uint8_t)f1(s1);
		*d++ ^= (uint8_t)f2(s2);
	}

	return p;
}


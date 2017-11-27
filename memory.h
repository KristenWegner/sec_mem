// memory.h - Inline static memory-related functions.


#include <stdbool.h>

#include "config.h"
#include "random.h"


#ifndef INCLUDE_MEMORY_H
#define INCLUDE_MEMORY_H 1


// Randomizes the specified block of memory p for the count of bytes n using the global random number 
// generator. Returns p.
inline static void* sec_memran(void* p, register size_t n)
{
	register uint8_t* t = (uint8_t*)p;

	while (n-- > 0U)
		*t++ = random_next_byte();

	return p;
}


// Performs p = p ^ q, for the given count of bytes. Returns p.
inline static void* sec_memxor(void *restrict p, const void *restrict q, register size_t n)
{
	register const uint8_t* s = (const uint8_t*)q;
	register uint8_t* d = (uint8_t*)p;

	for (; n > 0U; --n)
		*d++ ^= *s++;

	return p;
}


// Compares the first n bytes of two areas of memory. Returns zero if they are the same, a value less than 
// zero if p is lexically less than q, or a value greater than zero if p is lexically greater than q.
inline static int32_t sec_memcmp(const void *restrict p, const void *restrict q, register size_t n)
{
	register const uint8_t* l = (const uint8_t*)p;
	register const uint8_t* r = (const uint8_t*)q;

	while (n-- > 0U)
		if (*l++ != *r++)
			return (l[-1] < r[-1]) ? -1 : 1;
	
	return 0;
}


// Sets the first n bytes of p to the byte value v. Returns p.
inline static void* sec_memset(void *restrict p, register uint8_t v, register size_t n)
{
	register uint8_t* d = (uint8_t*)p;

	while (n-- > 0U)
		*d++ = v;

	return p;
}


// Returns the first occurrence of q of length n in p of length m. If not found, or m less than n, returns null. 
// Returns p if n is zero.
inline static void* sec_memmem(const void *restrict p, size_t m, const void *restrict q, register size_t n)
{
	register const uint8_t* b;
	register const uint8_t* const l = (((const uint8_t*)p) + m - n);

	if (n == 0) return (void*)p;
	if (m < n) return NULL;

	for (b = (const uint8_t*)p; b <= l; ++b)
		if (b[0] == ((const uint8_t*)q)[0] && !memcmp((const void*)&b[1], (const void*)((const uint8_t*)q + 1), n - 1))
			return (void*)b;

	return NULL;
}


// Searches the memory starting at p, up to length n bytes, for the first occurrence of the byte b. 
// Returns a pointer to the occurence of b in p if found, else null.
inline static void* sec_memchr(const void *restrict p, register uint8_t b, register size_t n)
{
	const uint8_t* t = (const uint8_t*)p;

	while (n-- > 0U)
	{
		if (*t == b) return (void*)t;
		t++;
	}

	return NULL;
}


// Copies n bytes from memory region q to region p. Returns p.
inline static void* sec_memcpy(void *restrict p, const void *restrict q, size_t n)
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


// Zeroes out the specified block of memory.
inline static void sec_memzer(void *restrict p, size_t n)
{
	volatile uint8_t* q = p;
	while (n--) *q++ = 0;
}


// Determines whether the memory block starting at s, of length m, from offset o, contains sufficient space for n elements of size e.
inline static bool sec_memcon(register const void *restrict s, register const void *restrict o, register size_t m, size_t e, size_t n)
{
	uint64_t k;

	if (!s || !o || o < s || (const uint8_t*)o >= (const uint8_t*)s + m) return false;

	uint64_t l = (uint64_t)((const uint8_t*)s + m);
	uint64_t d = (uint64_t)((const uint8_t*)o - (const uint8_t*)s);

	if (d >= (uint64_t)((const uint8_t*)s + m)) return false;

	k = (uint64_t)(e * n);

	return (uint64_t)((const uint8_t*)o + k) <= l;
}


#endif // INCLUDE_MEMORY_H


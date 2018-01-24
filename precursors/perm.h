// perm.h


#ifndef INCLUDE_PERM_H
#define INCLUDE_PERM_H 1


#include <stdint.h>
#include <stddef.h>
#include <limits.h>


// Returns a word rotated r bits to the left.
inline static uint64_t rotl(register uint64_t x, register uint64_t r)
{
	return  (x << r) | (x >> (UINT64_C(64) - r));
}


// Returns a word rotated r bits to the right.
inline static uint64_t rotr(register uint64_t x, register uint64_t r)
{
	return  (x >> r) | (x << (UINT64_C(64) - r));
}


// Returns a word with bits of w distributed as indicated by m.
inline static uint64_t scatter(register uint64_t w, register uint64_t m)
{
	register uint64_t i, z = UINT64_C(0), b = UINT64_C(1);

	while (m)
	{
		i = m & -m;
		m ^= i;
		z += (b & w) ? i : UINT64_C(0);
		b <<= UINT64_C(1);
	}

	return z;
}


// Returns a word with bits of w collected as indicated by m.
inline static uint64_t gather(register uint64_t w, register uint64_t m)
{
	register uint64_t i, z = 0, b = 1;

	while (m)
	{
		i = m & -m;
		m ^= i;
		z += (i & w) ? b : UINT64_C(0);
		b <<= UINT64_C(1);
	}

	return z;
}


// Returns the count of set bits.
inline static uint64_t count(register uint64_t x)
{
	x -= (x >> UINT64_C(1)) & UINT64_C(0x5555555555555555);
	x = ((x >> UINT64_C(2)) & UINT64_C(0x3333333333333333)) + (x & UINT64_C(0x3333333333333333));
	x = ((x >> UINT64_C(4)) + x) & UINT64_C(0x0F0F0F0F0F0F0F0F);
	x *= UINT64_C(0x0101010101010101);

	return (x >> 56);
}


// Returns the count of bit blocks.
inline static uint64_t blocks(register uint64_t x)
{
	return (x & UINT64_C(1)) + count((x ^ (x >> UINT64_C(1)))) / UINT64_C(2);
}


// Thue-Morse sequence.
inline static uint64_t thue_morse(register uint64_t* k, register uint64_t* s)
{
	if (*s == UINT64_C(0)) *s = parity(*k);

	register uint64_t x = *k ^ (*k + 1);

	++*k;
	x ^= x >> UINT64_C(1);
	x &= UINT64_C(0x5555555555555555);
	*s ^= (x != UINT64_C(0));

	return *s;
}


#endif // INCLUDE_PERM_H


// bits.h - Misc. bit manipulation functions.


#include "config.h"


#ifndef INCLUDE_BITS_H
#define INCLUDE_BITS_H 1


// Interleave XOR folds a 64-bit value into an 8-bit value.
inline static uint8_t sm_fold_64_to_8(register uint64_t v)
{
	register union { uint64_t q; uint8_t b[sizeof(uint64_t)]; } u = { .q = v };
	return (uint8_t)(u.b[7] ^ u.b[0] ^ u.b[6] ^ u.b[1] ^ u.b[5] ^ u.b[2] ^ u.b[4] ^ u.b[3]);
}


// Set bit in v at i.
inline static uint64_t sm_set_bit_64(uint64_t v, uint64_t i)
{
	return (v | (UINT64_C(1) << i));
}


// Clear bit in v at i.
inline static uint64_t sm_clear_bit_64(uint64_t v, uint64_t i)
{
	return (v & ~(UINT64_C(1) << i));
}


// 32-bit reduces v to [0..n). Can substitute for modulus in certain situations
inline static uint32_t sm_reduce_32(register uint32_t v, register uint32_t n)
{
	return ((uint64_t)v * (uint64_t)n) >> 32;
}


// 8-bit cyclic rotate bits left.
inline static uint8_t sm_rotl_8(register uint8_t n, register uint8_t c)
{
	const uint8_t mask = (CHAR_BIT * sizeof(n) - UINT8_C(1));
	c &= mask;
	return (n << c) | (n >> ((-c) & mask));
}


// 8-bit cyclic rotate bits right.
inline static uint8_t sm_rotr_8(register uint8_t n, register uint8_t c)
{
	const uint8_t mask = (CHAR_BIT * sizeof(n) - UINT8_C(1));
	c &= mask;
	return (n >> c) | (n << ((-c) & mask));
}


// 64-bit cyclic rotate bits left.
inline static uint64_t sm_rotl_64(register uint64_t n, register uint64_t c)
{
	const uint64_t mask = (uint64_t)((sizeof(n) * CHAR_BIT) - UINT64_C(1));
	c &= mask;
	return (n << c) | (n >> ((-c) & mask));
}


// 64-bit cyclic rotate bits right.
inline static uint64_t sm_rotr_64(register uint64_t n, register uint64_t c)
{
	const uint64_t mask = (uint64_t)((sizeof(n) * CHAR_BIT) - UINT64_C(1));
	c &= mask;
	return (n >> c) | (n << ((-c) & mask));
}


// Performs 64-bit bit-wise perfect shuffle.
inline static uint64_t sm_shuffle_64(register uint64_t v)
{
	register uint64_t t = (v ^ (v >> 16)) & UINT64_C(0x00000000FFFF0000);

	v = v ^ t ^ (t << 16);
	t = (v ^ (v >> 8)) & UINT64_C(0x0000FF000000FF00);
	v = v ^ t ^ (t << 8);
	t = (v ^ (v >> 4)) & UINT64_C(0x00F000F000F000F0);
	v = v ^ t ^ (t << 4);
	t = (v ^ (v >> 2)) & UINT64_C(0x0C0C0C0C0C0C0C0C);
	v = v ^ t ^ (t << 2);
	t = (v ^ (v >> 1)) & UINT64_C(0x2222222222222222);
	v = v ^ t ^ (t << 1);

	return v;
}


// Separate bits from a given integer 3 positions apart.
inline static uint64_t sm_split_by_3_32_to_64(uint32_t v)
{
	register uint64_t x = (uint64_t)(v & UINT32_C(0x1FFFFF));

	x = (x | x << 32) & UINT64_C(0x001F00000000FFFF);
	x = (x | x << 16) & UINT64_C(0x001F0000FF0000FF);
	x = (x | x <<  8) & UINT64_C(0x100F00F00F00F00F);
	x = (x | x <<  4) & UINT64_C(0x10C30C30C30C30C3);
	x = (x | x <<  2) & UINT64_C(0x1249249249249249);

	return x;
}


// Morton encode 3 32-bit values to one 64-bit value.
inline static uint64_t sm_morton_encode_3_32_to_64(uint32_t x, uint32_t y, uint32_t z)
{
	return UINT64_C(0) | sm_split_by_3_32_to_64(x) | sm_split_by_3_32_to_64(y) << 1 | sm_split_by_3_32_to_64(z) << 2;
}


// Performs 64-bit bit-wise unzip.
inline static uint64_t sm_unzip_64(register uint64_t v)
{
	register uint64_t u = (v >> 1) & UINT64_C(0x5555555555555555);

	v &= UINT64_C(0x5555555555555555);

	v = (v | (v >>  1)) & UINT64_C(0x3333333333333333);
	u = (u | (u >>  1)) & UINT64_C(0x3333333333333333);
	v = (v | (v >>  2)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
	u = (u | (u >>  2)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
	v = (v | (v >>  4)) & UINT64_C(0x00FF00FF00FF00FF);
	u = (u | (u >>  4)) & UINT64_C(0x00FF00FF00FF00FF);
	v = (v | (v >>  8)) & UINT64_C(0x0000FFFF0000FFFF);
	u = (u | (u >>  8)) & UINT64_C(0x0000FFFF0000FFFF);
	v = (v | (v >> 16)) & UINT64_C(0x00000000FFFFFFFF);
	u = (u | (u >> 16)) & UINT64_C(0x00000000FFFFFFFF);

	v |= (u << 32);

	return v;
}


// Computes the 64-bit invertible yellow code transform.
// The yellow code is a good candidate for 'randomization'.
inline static uint64_t sm_yellow_64(register uint64_t v)
{
	register uint64_t s = UINT64_C(64) >> 1;
	register uint64_t m = ~UINT64_C(0) >> s;

	do
	{
		v ^= ((v & m) << s);
		s >>= 1;
		m ^= (m << s);
	}
	while (s);

	return v;
}


// Computes the 64-bit invertible green code transform. 
inline static uint64_t sm_green_64(register uint64_t v)
{
	register uint64_t s = UINT64_C(64) >> 1;
	register uint64_t m = ~UINT64_C(0) << s;

	do
	{
		register uint64_t t = v & m;
		register uint64_t u = v ^ t;

		v = u ^ (t >> s);
		v ^= (u << s);
		s >>= 1;
		m ^= (m >> s);
	}
	while (s);

	return v;
}


// Computes the 64-bit invertible red code transform. 
inline static uint64_t sm_red_64(register uint64_t v)
{
	register uint64_t s = UINT64_C(64) >> 1;
	register uint64_t m = ~UINT64_C(0) >> s;

	do
	{
		register uint64_t x = v & m;
		register uint64_t y = v ^ x;

		v = y ^ (x << s);
		v ^= (y >> s);
		s >>= 1;
		m ^= (m << s);
	} 
	while (s);

	return v;
}


// Computes the 64-bit invertible blue code transform. 
inline static uint64_t sm_blue_64(register uint64_t v)
{
	register uint64_t s = UINT64_C(64) >> 1;
	register uint64_t m = ~UINT64_C(0) << s;

	do
	{
		v ^= ((v & m) >> s);
		s >>= 1;
		m ^= (m >> s);
	} 
	while (s);

	return v;
}


// Computes the 64-bit inverse, reverse gray code.
inline static uint64_t sm_inv_rev_gray_64(register uint64_t v)
{
	v ^= (v << 1);
	v ^= (v << 2);
	v ^= (v << 4);
	v ^= (v << 8);
	v ^= (v << 16);
	return v ^ (v << 32);
}


// Gets a value indicating whether the i-th bit of the 64-bit value, v, is set.
inline static uint8_t sm_is_bit_set_64(register uint64_t v, register uint64_t i)
{
	return ((v & (UINT64_C(1) << i)) != UINT64_C(0)) ? UINT8_C(1) : UINT8_C(0);
}


// Swap bytes, 64-bit.
inline static uint64_t sm_swap_64(register uint64_t v)
{
	return 
		((v & UINT64_C(0x00000000000000FF)) << 56) | 
		((v & UINT64_C(0x000000000000FF00)) << 40) | 
		((v & UINT64_C(0x0000000000FF0000)) << 24) | 
		((v & UINT64_C(0x00000000FF000000)) <<  8) | 
		((v & UINT64_C(0x000000FF00000000)) >>  8) | 
		((v & UINT64_C(0x0000FF0000000000)) >> 24) | 
		((v & UINT64_C(0x00FF000000000000)) >> 40) | 
		((v & UINT64_C(0xFF00000000000000)) >> 56);
}


inline static uint64_t sm_next_even_64(register uint64_t v) { return (v | UINT64_C(1)) + UINT64_C(1); }
inline static uint64_t sm_prev_even_64(register uint64_t v) { return (v - UINT64_C(1)) & ~UINT64_C(1); }
inline static uint64_t sm_next_odd_64(register uint64_t v) { return (v + UINT64_C(1)) | UINT64_C(1); }
inline static uint64_t sm_prev_odd_64(register uint64_t v) { return (v & ~UINT64_C(1)) - UINT64_C(1); }


// Gets the count of set bits.
inline static uint64_t sm_bit_count_64(register uint64_t v)
{
	v -= (v >> 1) & UINT64_C(0x5555555555555555);
	v = ((v >> 2) & UINT64_C(0x3333333333333333)) + (v & UINT64_C(0x3333333333333333));
	v = ((v >> 4) + v) & UINT64_C(0x0F0F0F0F0F0F0F0F);
	v *= UINT64_C(0x0101010101010101);
	return v >> 56;
}


// Gets the count of bit blocks.
inline static uint64_t sm_bit_blocks_64(register uint64_t v)
{
	register uint64_t u = v ^ (v >> 1);
	u -= (u >> 1) & UINT64_C(0x5555555555555555);
	u = ((u >> 2) & UINT64_C(0x3333333333333333)) + (u & UINT64_C(0x3333333333333333));
	u = ((u >> 4) + u) & UINT64_C(0x0F0F0F0F0F0F0F0F);
	u *= UINT64_C(0x0101010101010101);
	return (v & UINT64_C(1)) + (u >> 56) / UINT64_C(2);
}


// Returns floor(x + y / 2) even if x + y won't fit into a uint64_t.
inline static uint64_t sm_average_64(register uint64_t x, register uint64_t y)
{
	return  (x & y) + ((x ^ y) >> 1);
}


// Computes the leading count of 0-bits in v starting from the MSB.
inline static uint8_t sm_clz_64(uint64_t v)
{
	if (v == UINT64_C(0)) return UINT8_C(0);
	uint64_t m = UINT64_C(0x8000000000000000);
	uint8_t r = UINT8_C(0);
	while ((v & m) == UINT64_C(0) && r < UINT8_C(64)) m >>= 1, r++;
	return r;
}


// Computes the trailing count of 0-bits in v starting from the LSB.
inline static uint8_t sm_ctz_64(uint64_t v)
{
	if (v == UINT64_C(0)) return UINT8_C(0);
	v &= -v;
	uint8_t r = UINT8_C(0);
	while (v >>= 1) r++;
	return r;
}


inline static uint64_t sm_log2_64(uint64_t v)
{
	uint64_t r = UINT64_C(0);
	while (v >>= 1) r++;
	return r;
}


#endif // INCLUDE_BITS_H


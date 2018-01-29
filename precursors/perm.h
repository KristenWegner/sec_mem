// perm.h


#ifndef INCLUDE_PERM_H
#define INCLUDE_PERM_H 1


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>


#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#define inline __forceinline
#else
#define inline __inline__
#endif


// 8-bit cyclic rotate bits left.
inline static uint8_t rotl_8(register uint8_t value, register uint8_t bits)
{
	bits &= UINT8_C(7);
	return (value << bits) | (value >> ((-bits) & UINT8_C(7)));
}


// 8-bit cyclic rotate bits right.
inline static uint8_t rotr_8(register uint8_t value, register uint8_t bits)
{
	bits &= UINT8_C(7);
	return (value >> bits) | (value << ((-bits) & UINT8_C(7)));
}


// Cyclic rotate bits left.
inline static uint64_t rotl_64(register uint64_t value, register uint64_t bits)
{
	bits &= UINT64_C(63);
	return (value << bits) | (value >> ((-bits) & UINT64_C(63)));
}


// Cyclic rotate bits right.
inline static uint64_t rotr_64(register uint64_t value, register uint64_t bits)
{
	bits &= UINT64_C(63);
	return (value >> bits) | (value << ((-bits) & UINT64_C(63)));
}


// Rotate value left if bits < 0 or right if bits > 0 by the specified absolute count of bits, 
// otherwise (bits = 0) return value.
inline static uint64_t rot_64(register uint64_t value, register int64_t bits)
{
	bool left = (bits < INT64_C(0));
	register uint64_t dist = (uint64_t)(left ? -bits : bits);

	dist = (dist > UINT64_C(63)) ? dist % UINT64_C(64) : dist;

	if (!dist) return value;
	else if (left) return rotl_64(value, dist);
	return rotr_64(value, dist);
}


// Performs 64-bit bit-wise perfect shuffle.
inline static uint64_t shuffle_64(register uint64_t value)
{
	register uint64_t temp = (value ^ (value >> 16)) & UINT64_C(0x00000000FFFF0000);

	value = value ^ temp ^ (temp << 16);
	temp = (value ^ (value >> 8)) & UINT64_C(0x0000FF000000FF00);
	value = value ^ temp ^ (temp << 8);
	temp = (value ^ (value >> 4)) & UINT64_C(0x00F000F000F000F0);
	value = value ^ temp ^ (temp << 4);
	temp = (value ^ (value >> 2)) & UINT64_C(0x0C0C0C0C0C0C0C0C);
	value = value ^ temp ^ (temp << 2);
	temp = (value ^ (value >> 1)) & UINT64_C(0x2222222222222222);
	value = value ^ temp ^ (temp << 1);

	return value;
}


// Returns a word with bits of value distributed as indicated by mask.
inline static uint64_t scatter_64(register uint64_t value, register uint64_t mask)
{
	register uint64_t bit = UINT64_C(1), result = UINT64_C(0);

	while (mask)
	{
		register uint64_t temp = mask & -mask;
		mask ^= temp;
		result += (bit & value) ? temp : UINT64_C(0);
		bit <<= UINT64_C(1);
	}

	return result;
}


// Returns a word with bits of value collected as indicated by mask.
inline static uint64_t gather_64(register uint64_t value, register uint64_t mask)
{
	register uint64_t result = UINT64_C(0), bit = UINT64_C(1);

	while (mask)
	{
		register uint64_t temp = mask & -mask;
		mask ^= temp;
		result += (temp & value) ? bit : UINT64_C(0);
		bit <<= UINT64_C(1);
	}

	return result;
}


// Returns the count of set bits.
inline static uint64_t bits_64(register uint64_t value)
{
	value -= (value >> UINT64_C(1)) & UINT64_C(0x5555555555555555);
	value = ((value >> UINT64_C(2)) & UINT64_C(0x3333333333333333)) + (value & UINT64_C(0x3333333333333333));
	value = ((value >> UINT64_C(4)) + value) & UINT64_C(0x0F0F0F0F0F0F0F0F);
	value *= UINT64_C(0x0101010101010101);

	return (value >> 56);
}


// Returns the count of bit blocks.
inline static uint64_t blocks_64(register uint64_t value)
{
	return (value & UINT64_C(1)) + bits_64((value ^ (value >> UINT64_C(1)))) / UINT64_C(2);
}


// Return floor((value1 + value2) / 2) even if (value1 + value2) doesn't fit into a uint64_t.
inline static uint64_t avg_64(register uint64_t value1, register uint64_t value2)
{
	return  (value1 & value2) + ((value1 ^ value2) >> UINT64_C(1));
}


// Returns the Gray code, e.g. the bit-wise derivative modulo 2.
inline static uint64_t gray_64(register uint64_t value)
{
	return  value ^ (value >> UINT64_C(1));
}


// Returns the inverse Gray code, e.g. at each bit position, the parity of all bits of the 
// input to the left from it (including itself).
inline static uint64_t inv_gray_64(register uint64_t value)
{
	value ^= value >> UINT64_C(1);
	value ^= value >> UINT64_C(2);
	value ^= value >> UINT64_C(4);
	value ^= value >> UINT64_C(8);
	value ^= value >> UINT64_C(16);
	value ^= value >> UINT64_C(32);

	return value;
}


// Return 0 if the number of set bits is even, else 1.
inline static uint64_t parity_64(register uint64_t value)
{
	return inv_gray_64(value) & UINT64_C(1);
}


// Converts a binary number to the corresponding word in SL-Gray order.
inline static uint64_t bin_sl_gray_64(register uint64_t value, register uint64_t ldn)
{
	if (ldn == UINT64_C(0))
		return UINT64_C(0);

	register uint64_t b = UINT64_C(1) << (ldn - UINT64_C(1));
	register uint64_t m = (b << UINT64_C(1)) - UINT64_C(1);
	register uint64_t result = b;

	value -= UINT64_C(1);

	while (b != UINT64_C(0))
	{
		register const uint64_t h = value & b;

		result ^= h;

		if (!h)
			value ^= m;

		value += UINT64_C(1);
		b >>= UINT64_C(1);
		m >>= UINT64_C(1);
	}

	return result;
}


// Converts a binary word in SL-Gray order to a binary number.
inline static uint64_t sl_gray_bin_64(register uint64_t value, register uint64_t ldn)
{
	if (value == UINT64_C(0)) 
		return UINT64_C(0);

	register uint64_t b = UINT64_C(1) << (ldn - UINT64_C(1));
	register uint64_t h = value & b;

	value ^= h;

	register uint64_t z = sl_gray_bin_64(value, ldn - UINT64_C(1));

	if (h == UINT64_C(0)) 
		return (b << UINT64_C(1)) - z;
	else return UINT64_C(1) + z;
}


// Returns the next value in the Thue-Morse sequence of the given value and state.
// If first is true then the state is initialized.
inline static uint64_t thue_morse_64(register uint64_t* value, register uint64_t* state, bool first)
{
	if (first) *state = parity_64(*value);

	register uint64_t x = *value ^ (*value + 1);

	++*value;
	x ^= x >> UINT64_C(1);
	x &= UINT64_C(0x5555555555555555);
	*state ^= (x != UINT64_C(0));

	return *state;
}


// Returns the inverse modulo 2 ^ 64 of the input. 
// Restrictions: Input must be odd.
inline static uint64_t inv_adic_64(register uint64_t value)
{
	if ((value & UINT64_C(1)) == UINT64_C(0))
		return UINT64_C(0);

	uint64_t p, result = value;

	do
	{
		p = result * value;
		result *= (UINT64_C(2) - p);
	}
	while (p != UINT64_C(1));

	return result;
}


// Returns the inverse square root modulo 2 ^ 64. 
// Restrictions: Input must have value = 1 mod 8.
inline static uint64_t inv_sqrt_adic_64(register uint64_t value)
{
	if ((value & UINT64_C(7)) != UINT64_C(1))
		return UINT64_C(0);

	uint64_t p, t, result = (value >> UINT64_C(1)) | UINT64_C(1);

	do
	{
		t = result;
		p = (UINT64_C(3) - value * t * t);
		result = (t * p) >> UINT64_C(1);
	}
	while (result != t);

	return result;
}


// Returns the square root modulo 2 ^ 64.
// Restrictions: Input must have value = 1 mod 8, value = 4 mod 32, value = 16 mod 128, etc.
inline static uint64_t sqrt_adic_64(register uint64_t value)
{
	if (value == UINT64_C(0))
		return UINT64_C(0);

	register uint64_t s = UINT64_C(0);

	while ((value & UINT64_C(1)) == UINT64_C(0))
		value >>= UINT64_C(1), ++s;

	value *= inv_sqrt_adic_64(value);
	value <<= (s >> UINT64_C(1));

	return value;
}


// Returns whether floor(log2(value1)) == floor(log2(value2)).
inline static bool floor_log2_equ_64(register uint64_t value1, register uint64_t value2)
{
	return  ((value1 ^ value2) <= (value1 & value2));
}


// Returns whether floor(log2(value1)) != floor(log2(value2)).
inline static bool floor_log2_neq_64(register uint64_t value1, register uint64_t value2)
{
	return  ((value1 ^ value2) > (value1 & value2));
}


// Returns the index of the highest set bit, or 0 if no bit is set.
inline static uint64_t highest_set_64(register uint64_t value)
{
	return
		   (uint64_t)floor_log2_neq_64(value, value & UINT64_C(0x5555555555555555))
		+ ((uint64_t)floor_log2_neq_64(value, value & UINT64_C(0x3333333333333333)) << UINT64_C(1))
		+ ((uint64_t)floor_log2_neq_64(value, value & UINT64_C(0x0F0F0F0F0F0F0F0F)) << UINT64_C(2))
		+ ((uint64_t)floor_log2_neq_64(value, value & UINT64_C(0x00FF00FF00FF00FF)) << UINT64_C(3))
		+ ((uint64_t)floor_log2_neq_64(value, value & UINT64_C(0x0000FFFF0000FFFF)) << UINT64_C(4))
		+ ((uint64_t)floor_log2_neq_64(value, value & UINT64_C(0x00000000FFFFFFFF)) << UINT64_C(5));
}


// Returns floor(log2(value)), i.e. return k so that 2 ^ k <= x < 2 ^ (k + 1). 
// If x = 0, then 0 is returned.
inline static uint64_t floor_log2_64(register uint64_t value)
{
	return highest_set_64(value);
}


// Returns true if value = 0 or value = 2 ^ k.
inline static bool is_pow2_64(register uint64_t value)
{
	return !(value & (value - UINT64_C(1)));
}


// Returns true if value is in [1, 2, 4, 8, 16, ..].
inline static bool one_bit_q_64(register uint64_t value)
{
	register uint64_t m = value - UINT64_C(1);
	return (((value ^ m) >> UINT64_C(1)) == m);
}


// Returns value if value = 2 ^ k, else returns 2 ^ ceil(log2(value)). 
// Exceptions: Returns 0 for value = 0.
inline static uint64_t next_pow_two_64(register uint64_t value)
{
	if (is_pow2_64(value))
		return value;

	value |= value >> UINT64_C(1);
	value |= value >> UINT64_C(2);
	value |= value >> UINT64_C(4);
	value |= value >> UINT64_C(8);
	value |= value >> UINT64_C(16);
	value |= value >> UINT64_C(32);

	return value + UINT64_C(1);
}


// Returns k if value = 2 ^ k, else k + 1. Exception: returns 0 for value = 0.
inline static uint64_t next_exp2_64(register uint64_t value)
{
	if (value <= UINT64_C(1)) return UINT64_C(0);
	return floor_log2_64(value - UINT64_C(1)) + UINT64_C(1);
}


// Returns the minimal bit count of (t ^ value2) where t runs through the cyclic rotations of value1.
inline static uint64_t cyc_dist_64(register uint64_t value1, register uint64_t value2)
{
	register uint64_t result = ~UINT64_C(0), t = value1;

	do
	{
		register uint64_t z = t ^ value2;
		register uint64_t e = bits_64(z);

		if (e < result) 
			result = e;

		t = rotr_64(t, UINT64_C(1));
	}
	while (t != value1);

	return result;
}


// Swaps the two central blocks of 16 bits.
inline static uint64_t butterfly_16_64(register uint64_t value)
{
	return (value & UINT64_C(0xFFFF0000FC0003FF)) | ((value & UINT64_C(0xFFFF00000000)) >> UINT64_C(16)) | 
		((value & UINT64_C(0x3FFFC00)) << UINT64_C(16));
}


// Swaps in each block of 32 bits the two central blocks of 8 bits.
inline static uint64_t butterfly_8_64(register uint64_t value)
{
	return (value & ~UINT64_C(0xFFFF0000FFFF00)) | ((value & UINT64_C(0xFF000000FF0000)) >> UINT64_C(8)) | 
		((value & UINT64_C(0xFF000000FF00)) << UINT64_C(8));
}


// Swaps in each block of 16 bits the two central blocks of 4 bits.
inline static uint64_t butterfly_4_64(register uint64_t value)
{
	return (value & ~UINT64_C(0xFF00FF00FF00FF0)) | ((value & UINT64_C(0x0F000F000F000F00)) >> UINT64_C(4)) | 
		((value & UINT64_C(0xF000F000F000F0)) << UINT64_C(4));
}


// Swaps in each block of 8 bits the two central blocks of 2 bits.
inline static uint64_t butterfly_2_64(register uint64_t value)
{
	return (value & ~UINT64_C(0x3C3C3C3C3C3C3C3C)) | ((value & UINT64_C(0x3030303030303030)) >> UINT64_C(2)) | 
		((value & UINT64_C(0xC0C0C0C0C0C0C0C)) << UINT64_C(2));
}


// Returns the first combination (smallest word with) k bits, i.e. 00..001111..1 (low bits set). 
// Restrictions: Must have 0 <= value <= 64.
inline static uint64_t first_comb_64(register uint64_t value)
{
	if (value == UINT64_C(0)) return UINT64_C(0);
	return ~UINT64_C(0) >> (UINT64_C(16) - value);
}


// Returns the last combination of (biggest n-bit word with) k bits, i.e. 1111..100..00 (k high bits set).
// Restrictions: Must have 0 <= value <= bits <= 64.
inline static uint64_t last_comb_64(register uint64_t value, register uint64_t bits)
{
	return  first_comb_64(value) << (bits - value);
}


// Return smallest integer greater than value with the same number of bits set.
inline static uint64_t next_colex_comb_64(register uint64_t value)
{
	register uint64_t z;

	if (value & UINT64_C(1))
	{
		z = ~value;
		z &= -z;

		if (z == UINT64_C(0)) 
			return UINT64_C(0);

		value += (z >> UINT64_C(1));

		return value;
	}

	z = value & -value;

	register uint64_t v = value + z;

	if (v)
		v += (v ^ value) / (z >> UINT64_C(2));

	return v;
}


// Returns the inverse of the next colexicographic combination.
static inline uint64_t prev_colex_comb_64(register uint64_t value)
{
	value = next_colex_comb_64(~value);

	if (value != UINT64_C(0)) 
		value = ~value;

	return value;
}


// Returns the inverse Gray code of the next combination in minimal-change order.
// Restrictions: The input must be the inverse Gray code of the current combination.
inline static uint64_t inv_gray_next_min_change_64(register uint64_t value)
{
	register uint64_t ga = gray_64(value);
	register uint64_t i = UINT64_C(2);

	do
	{
		register uint64_t y = value + i;

		i <<= 1;

		register uint64_t gb = gray_64(y);
		register uint64_t r = ga ^ gb;

		if (is_pow2_64(r & gb) && is_pow2_64(r & ga))
			return y;
	}
	while (true);

	return UINT64_C(0);
}


// Returns the inverse Gray code of the previous combination in minimal-change order.
// Restrictions: The input must be the inverse Gray code of the current combination.
inline static uint64_t inv_gray_prev_min_change_64(register uint64_t value1, register uint64_t value2)
{
	uint64_t result, i = UINT64_C(1);

	do
	{
		i <<= UINT64_C(1);
		result = value1 - i;
	}
	while (bits_64(gray_64(result)) != value2);

	return result;
}


// Returns result if value1 == rotr(value2, result), else return ~0, e.g. how many times the 
// value2 argument must be rotated right in order to match value1.
inline static uint64_t cyc_match_64(register uint64_t value1, register uint64_t value2)
{
	register uint64_t result = UINT64_C(0);

	do
	{
		if (value1 == value2)
			return result;

		value2 = rotr_64(value2, UINT64_C(1));
	} 
	while (++result < UINT64_C(64));

	return ~UINT64_C(0);
}


// Returns the minimal value such that that c = value1 ^ rotr(value2, result, n) is a one-bit word, 
// or ~0 if there is no such result.
inline static uint64_t cyc_dist_match(register uint64_t value1, register uint64_t value2)
{
	register uint64_t result = UINT64_C(0);

	do
	{
		if (one_bit_q_64(value1 ^ value2))
			return result;

		value2 = rotr_64(value2, UINT64_C(1));
	}
	while (++result < UINT64_C(64));

	return ~UINT64_C(0);
}


// Copies the bit from value[source] to value[destination], returning the modified value.
inline static uint64_t copy_bit_64(register uint64_t value, register uint64_t source, register uint64_t destination)
{
	return value ^ ((((value >> source) ^ (value >> destination)) & UINT64_C(1)) << destination);
}


// Copies a single bit, according to mask1, to the bit according to mask2, returning the modified value.
// Restrictions: Both mask1 and mask2 may have only a single bit set.
inline static uint64_t mask_copy_64(register uint64_t value, register uint64_t mask1, register uint64_t mask2)
{
	register uint64_t temp = mask2;

	if (mask1 & value)
		temp = UINT64_C(0);

	temp ^= mask2;
	value &= ~mask2;
	value |= temp;

	return value;
}


// Returns the minimum value of all cyclic rotations of the specified value.
inline static uint64_t cyc_min_64(register uint64_t value)
{
	register uint64_t temp = UINT64_C(1), result = value;

	do
	{
		value = rotr_64(value, UINT64_C(1));

		if (value < result)
			result = value;
	}
	while (++temp < UINT64_C(64));

	return result;
}


// Returns the maximum value of all cyclic rotations of the specified value.
inline static uint64_t cyc_max_64(register uint64_t value)
{
	register uint64_t temp = UINT64_C(1), result = value;

	do
	{
		value = rotr_64(value, UINT64_C(1));

		if (value > result)
			result = value;
	}
	while (++temp < UINT64_C(64));

	return result;
}


// Returns the minimal positive bit-rotation that transforms the given value into itself.
static inline uint64_t cyc_period_64(register uint64_t value)
{
	register uint64_t result = UINT64_C(1);

	do
	{
		if (value == rotr_64(value, result))
			return result;

		result <<= 1;
	}
	while (result < UINT64_C(64));

	return result;
}


// Similar to the Gray code, except the right-most shifted bit is moved to the highest bit
// position. The returned value will have an even number of bits set.
inline static uint64_t cyc_rxor_64(register uint64_t value)
{
	return value ^ rotr_64(value, UINT64_C(1));
}


// Similar to the Gray code, except the left-most shifted bit is moved to the lowest bit
// position. The returned value will have an even number of bits set.
inline static uint64_t cyc_lxor_64(register uint64_t value)
{
	return value ^ rotl_64(value, UINT64_C(1));
}


#endif // INCLUDE_PERM_H


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


// Returns a word rotated n bits to the left.
inline static uint64_t rotate_left_64(register uint64_t value, register uint64_t bits)
{
	return  (value << bits) | (value >> (UINT64_C(64) - bits));
}


// Returns a word rotated n bits to the right.
inline static uint64_t rotate_right_64(register uint64_t value, register uint64_t bits)
{
	return  (value >> bits) | (value << (UINT64_C(64) - bits));
}


// Rotate value left or right by the specified count of bits.
inline static uint64_t rotate_64(register uint64_t value, register int64_t bits)
{
	if (bits < 0) return rotate_left_64(value, -bits);
	else if (bits > 0) return rotate_right_64(value, bits);
	else return UINT64_C(0);
}


// Returns a word with bits of value distributed as indicated by mask.
inline static uint64_t scatter_64(register uint64_t value, register uint64_t mask)
{
	register uint64_t i, b = UINT64_C(1), z = UINT64_C(0);

	while (mask)
	{
		i = mask & -mask;
		mask ^= i;
		z += (b & value) ? i : UINT64_C(0);
		b <<= UINT64_C(1);
	}

	return z;
}


// Returns a word with bits of value collected as indicated by mask.
inline static uint64_t gather_64(register uint64_t value, register uint64_t mask)
{
	register uint64_t i, z = 0, b = 1;

	while (mask)
	{
		i = mask & -mask;
		mask ^= i;
		z += (i & value) ? b : UINT64_C(0);
		b <<= UINT64_C(1);
	}

	return z;
}


// Returns the count of set bits.
inline static uint64_t count_set_bits_64(register uint64_t x)
{
	x -= (x >> UINT64_C(1)) & UINT64_C(0x5555555555555555);
	x = ((x >> UINT64_C(2)) & UINT64_C(0x3333333333333333)) + (x & UINT64_C(0x3333333333333333));
	x = ((x >> UINT64_C(4)) + x) & UINT64_C(0x0F0F0F0F0F0F0F0F);
	x *= UINT64_C(0x0101010101010101);

	return (x >> 56);
}


// Returns the count of bit blocks.
inline static uint64_t count_bit_blocks_64(register uint64_t value)
{
	return (value & UINT64_C(1)) + count_set_bits_64((value ^ (value >> UINT64_C(1)))) / UINT64_C(2);
}


// Return floor((value1 + value2) / 2) even if (value1 + value2) doesn't fit into a uint64_t.
inline static uint64_t average_64(register uint64_t value1, register uint64_t value2)
{
	return  (value1 & value2) + ((value1 ^ value2) >> UINT64_C(1));
}


// Returns the Gray code, e.g. the bit-wise derivative modulo 2.
inline static uint64_t gray_code_64(register uint64_t value)
{
	return  value ^ (value >> UINT64_C(1));
}


// Returns the inverse Gray code, e.g. at each bit position, the parity of all bits of the 
// input to the left from it (including itself).
inline static uint64_t inverse_gray_code_64(register uint64_t value)
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
	return inverse_gray_code_64(value) & UINT64_C(1);
}


// Converts a binary number to the corresponding word in SL-Gray order.
inline static uint64_t bin_to_sl_gray_64(register uint64_t value, register uint64_t ldn)
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
inline static uint64_t sl_gray_to_bin_64(register uint64_t value, register uint64_t ldn)
{
	if (value == UINT64_C(0)) 
		return UINT64_C(0);

	register uint64_t b = UINT64_C(1) << (ldn - UINT64_C(1));
	register uint64_t h = value & b;

	value ^= h;

	register uint64_t z = sl_gray_to_bin_64(value, ldn - UINT64_C(1));

	if (h == UINT64_C(0)) 
		return (b << UINT64_C(1)) - z;
	else return UINT64_C(1) + z;
}


// Returns the next value in the Thue-Morse sequence of the given value and state.
// If first is true then the state is initialized.
inline static uint64_t thue_morse_next_64(register uint64_t* value, register uint64_t* state, bool first)
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
inline static uint64_t inverse_to_adic_64(register uint64_t value)
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
inline static uint64_t inverse_sqrt_to_adic_64(register uint64_t value)
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


// Returns the square root modulo 2 ^ 64. Restrictions: Input must have value = 1 mod 8, 
// value = 4 mod 32, value = 16 mod 128, etc.
inline static uint64_t sqrt_to_adic_64(register uint64_t value)
{
	if (value == UINT64_C(0))
		return UINT64_C(0);

	register uint64_t s = UINT64_C(0);

	while ((value & UINT64_C(1)) == UINT64_C(0))
		value >>= UINT64_C(1), ++s;

	value *= inverse_sqrt_to_adic_64(value);
	value <<= (s >> UINT64_C(1));

	return value;
}


// Returns whether floor(log2(value1)) == floor(log2(value2)).
inline static bool floor_log_two_equal_64(register uint64_t value1, register uint64_t value2)
{
	return  ((value1 ^ value2) <= (value1 & value2));
}


// Returns whether floor(log2(value1)) != floor(log2(value2)).
inline static bool floor_log_two_not_equal_64(register uint64_t value1, register uint64_t value2)
{
	return  ((value1 ^ value2) > (value1 & value2));
}


// Returns the index of the highest set bit, or 0 if no bit is set.
inline static uint64_t index_highest_set_64(register uint64_t value)
{
	return
		   (uint64_t)floor_log_two_not_equal_64(value, value & UINT64_C(0x5555555555555555))
		+ ((uint64_t)floor_log_two_not_equal_64(value, value & UINT64_C(0x3333333333333333)) << UINT64_C(1))
		+ ((uint64_t)floor_log_two_not_equal_64(value, value & UINT64_C(0x0F0F0F0F0F0F0F0F)) << UINT64_C(2))
		+ ((uint64_t)floor_log_two_not_equal_64(value, value & UINT64_C(0x00FF00FF00FF00FF)) << UINT64_C(3))
		+ ((uint64_t)floor_log_two_not_equal_64(value, value & UINT64_C(0x0000FFFF0000FFFF)) << UINT64_C(4))
		+ ((uint64_t)floor_log_two_not_equal_64(value, value & UINT64_C(0x00000000FFFFFFFF)) << UINT64_C(5));
}


// Returns floor(log2(value)), i.e. return k so that 2 ^ k <= x < 2 ^ (k + 1). 
// If x = 0, then 0 is returned.
inline static uint64_t floor_log_two_64(register uint64_t value)
{
	return index_highest_set_64(value);
}


// Returns true if value = 0 or value = 2 ^ k.
inline static bool is_power_of_two_64(register uint64_t value)
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
inline static uint64_t next_power_of_two_64(register uint64_t value)
{
	if (is_power_of_two_64(value))
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
inline static uint64_t next_exponent_of_two(register uint64_t value)
{
	if (value <= UINT64_C(1)) return UINT64_C(0);
	return floor_log_two_64(value - UINT64_C(1)) + UINT64_C(1);
}


// Returns the minimal bit count of (t ^ value2) where t runs through the cyclic rotations of value1.
inline static uint64_t cyclic_distance_64(register uint64_t value1, register uint64_t value2)
{
	register uint64_t result = ~UINT64_C(0), t = value1;

	do
	{
		register uint64_t z = t ^ value2;
		register uint64_t e = count_set_bits_64(z);

		if (e < result) 
			result = e;

		t = rotate_right_64(t, UINT64_C(1));
	}
	while (t != value1);

	return result;
}


// Swaps the two central blocks of 16 bits.
inline static uint64_t butterfly_16_64(register uint64_t value)
{
	return (value & UINT64_C(0xFFFF0000FC0003FF)) | ((value & UINT64_C(0xFFFF00000000)) >> UINT64_C(16)) | ((value & UINT64_C(0x3FFFC00)) << UINT64_C(16));
}


// Swaps in each block of 32 bits the two central blocks of 8 bits.
inline static uint64_t butterfly_8_64(register uint64_t value)
{
	return (value & ~UINT64_C(0xFFFF0000FFFF00)) | ((value & UINT64_C(0xFF000000FF0000)) >> UINT64_C(8)) | ((value & UINT64_C(0xFF000000FF00)) << UINT64_C(8));
}


// Swaps in each block of 16 bits the two central blocks of 4 bits.
inline static uint64_t butterfly_4_64(register uint64_t value)
{
	return (value & ~UINT64_C(0xFF00FF00FF00FF0)) | ((value & UINT64_C(0x0F000F000F000F00)) >> UINT64_C(4)) | ((value & UINT64_C(0xF000F000F000F0)) << UINT64_C(4));
}


// Swaps in each block of 8 bits the two central blocks of 2 bits.
inline static uint64_t butterfly_2_64(register uint64_t value)
{
	return (value & ~UINT64_C(0x3C3C3C3C3C3C3C3C)) | ((value & UINT64_C(0x3030303030303030)) >> UINT64_C(2)) | ((value & UINT64_C(0xC0C0C0C0C0C0C0C)) << UINT64_C(2));
}


// Returns the first combination (smallest word with) k bits, i.e. 00..001111..1 (low bits set). 
// Restrictions: Must have 0 <= value <= 64.
inline static uint64_t first_combination_64(register uint64_t value)
{
	if (value == UINT64_C(0)) return UINT64_C(0);
	return ~UINT64_C(0) >> (UINT64_C(16) - value);
}


// Returns the last combination of (biggest n-bit word with) k bits, i.e. 1111..100..00 (k high bits set).
// Restrictions: Must have 0 <= value <= bits <= 64.
inline static uint64_t last_combination_64(register uint64_t value, register uint64_t bits)
{
	return  first_combination_64(value) << (bits - value);
}


// Return smallest integer greater than value with the same number of bits set.
inline static uint64_t next_colexicographic_combination_64(register uint64_t value)
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
static inline uint64_t previous_colexicographic_combination_64(register uint64_t value)
{
	value = next_colexicographic_combination_64(~value);

	if (value != UINT64_C(0)) 
		value = ~value;

	return value;
}


#endif // INCLUDE_PERM_H


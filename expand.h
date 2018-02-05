// expand.h - Dispersal of a single byte into a randomized uint64_t according to a RRNG stream and PRNG stream-derived mapping function.

#ifndef INCLUDE_EXPAND_H
#define INCLUDE_EXPAND_H 1

#include "sm.h"

// Returns the truth-value of the bit at byte value[index].
inline static bool sm_get_bit(register uint8_t value, register uint8_t index)
{
	return ((value & (UINT8_C(1) << index)) != UINT8_C(0));
}


// Returns a byte with the bit value[index] set to state.
inline static uint8_t sm_set_bit(register uint8_t value, register uint8_t index, register bool state)
{
	return (state) ? (value | (UINT8_C(1) << index)) : (value & ~(UINT8_C(1) << index));
}


// Randomize a uint64_t with the specified RRNG, then, according to a mapping function generated from the current state of the PRNG, 
// disperse the bits in the given byte (value) into an uint64_t, finally returning the uint64_t.
inline static uint64_t sm_disperse(uint8_t value, sm_get64_f rrng, sm_ran64_f prng, void* state)
{
	union { uint64_t word; uint8_t byte[8]; } data = { rrng() }; // To receive the dispersed bits, prepared with an entropic value.
	bool used[64] = // Track used-up slots.
	{
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	};
	
	register uint8_t i;

	for (i = 0; i < 8; ++i) // Perform the dispersal.
	{
		register uint64_t next; // The next PRNG random value.
		register uint8_t slot; // The next slot in [0..63].

		do // Search for an open slot.
		{
			next = prng(state);
			slot = (uint8_t)((next) ? next % UINT64_C(64) : next); // Place it in the range.
		}
		while (used[slot]); 

		used[slot] = true; // Mark it as used.

		register uint8_t byte = (slot > 7) ? slot / 8 : 0; // Which byte.
		register uint8_t bit = (byte && slot) ? slot % 8 : 0; // Which bit in byte.

		data.byte[byte] = sm_set_bit(data.byte[byte], bit, sm_get_bit(value, i)); // Get it and set it.
	}

	return data.word;
}

// Coalesces the given uint64_t (value) back into a uint8_t according to a mapping function generated from the current state 
// of the PRNG, finally returning the uint8_t.
inline static uint8_t sm_coalesce(uint64_t value, sm_get64_f rrng, sm_ran64_f prng, void* state)
{
	union { uint64_t word; uint8_t byte[8]; } data = { value }; // To gather the dispersed bits.
	bool used[64] = // Track used-up slots.
	{
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	};

	register uint8_t result = 0;
	register uint8_t i;

	for (i = 0; i < 8; ++i) // Perform the coalescence.
	{
		register uint64_t next; // The next PRNG random value.
		register uint8_t slot; // The next slot in [0..63].

		do // Search for an open slot.
		{
			next = prng(state);
			slot = (uint8_t)((next) ? next % UINT64_C(64) : next); // Place it in the range.
		}
		while (used[slot]);

		used[slot] = true; // Mark it as used.

		register uint8_t byte = (slot > 7) ? slot / 8 : 0; // Which byte.
		register uint8_t bit = (byte && slot) ? slot % 8 : 0; // Which bit in byte.

		result = sm_set_bit(result, i, sm_get_bit(data.byte[byte], bit)); // Get it and set it.
	}

	return result;
}


#endif // INCLUDE_EXPAND_H



// expand.h - Dispersal of a single byte into a randomized uint64_t according to a RRNG stream and PRNG stream-derived mapping function.

#ifndef INCLUDE_EXPAND_H
#define INCLUDE_EXPAND_H 1

#include "sm.h"


#define SM_DISP_FALSE_32 { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false }
#define SM_DISP_FALSE_64 { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false }


// Returns the truth-value of the bit at byte value[index].
inline static bool sm_disp_get_bit(register uint8_t value, register uint8_t index)
{
	return ((value & (UINT8_C(1) << index)) != UINT8_C(0));
}


// Returns a byte with the bit value[index] set to state.
inline static uint8_t sm_disp_set_bit(register uint8_t value, register uint8_t index, register bool state)
{
	return (state) ? (value | (UINT8_C(1) << index)) : (value & ~(UINT8_C(1) << index));
}


// Cyclic rotate bits right.
inline static uint64_t sm_disp_rotr_64(register uint64_t value, register uint64_t bits)
{
	bits &= UINT64_C(63);
	return (value >> bits) | (value << ((-bits) & UINT64_C(63)));
}


// Randomize a uint64_t with the specified RRNG, then, according to a mapping function generated from the current state of the PRNG, 
// disperse the bits in the given byte (value) into an uint64_t, finally returning the uint64_t.
exported uint64_t callconv sm_disp_disperse_64(uint8_t value, sm_get64_f rrng, sm_ran64_f prng, void* state)
{
	union { uint64_t word; uint8_t byte[8]; } data = { rrng() }; // To receive the dispersed bits, prepared with an entropic value.
	bool used[64] = SM_DISP_FALSE_64; // Track used-up slots.	
	register uint8_t i;

	for (i = 0; i < UINT8_C(8); ++i) // Perform the dispersal.
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

		register uint8_t byte = (slot > UINT8_C(7)) ? slot / UINT8_C(8) : UINT8_C(0); // Which byte.
		register uint8_t bit = (byte && slot) ? slot % UINT8_C(8) : UINT8_C(0); // Which bit in byte.

		data.byte[byte] = sm_disp_set_bit(data.byte[byte], bit, sm_disp_get_bit(value, i)); // Get it and set it.
	}

	return data.word;
}


// Coalesces the given uint64_t (value) back into a uint8_t according to a mapping function generated from the current state 
// of the PRNG, finally returning the uint8_t.
exported uint8_t callconv sm_disp_coalesce_64(uint64_t value, sm_get64_f rrng, sm_ran64_f prng, void* state)
{
	union { uint64_t word; uint8_t byte[8]; } data = { value }; // To gather the dispersed bits.
	bool used[64] = SM_DISP_FALSE_64; // Track used-up slots.
	register uint8_t result = UINT8_C(0);
	register uint8_t i;

	for (i = 0; i < UINT8_C(8); ++i) // Perform the coalescence.
	{
		register uint64_t next; // The next PRNG random value.
		register uint8_t slot; // The next slot in [0..63].
		register uint8_t byte = 0; // Which byte.
		register uint8_t bit = 0; // Which bit in byte.

		do // Search for an open slot.
		{
			next = prng(state);
			slot = (uint8_t)((next) ? next % UINT64_C(64) : next); // Place it in the range.
		}
		while (used[slot]);

		used[slot] = true; // Mark it as used.

		register uint8_t byte = (slot > UINT8_C(7)) ? slot / UINT8_C(8) : UINT8_C(0); // Which byte.
		register uint8_t bit = (byte && slot) ? slot % UINT8_C(8) : UINT8_C(0); // Which bit in byte.

		result = sm_disp_set_bit(result, i, sm_disp_get_bit(data.byte[byte], bit)); // Get it and set it.
	}

	return result;
}


// Randomize a uint32_t with the specified RRNG, then, according to a mapping function generated from the current state of the PRNG, 
// disperse the bits in the given byte (value) into an uint32_t, finally returning the uint32_t.
exported uint32_t callconv sm_disp_disperse_32(uint8_t value, sm_get64_f rrng, sm_ran64_f prng, void* state)
{
	union { uint32_t word; uint8_t byte[4]; } data = { rrng() }; // To receive the dispersed bits, prepared with an entropic value.
	bool used[32] = SM_DISP_FALSE_32; // Track used-up slots.	
	register uint8_t i;

	for (i = 0; i < UINT8_C(8); ++i) // Perform the dispersal.
	{
		register uint64_t next; // The next PRNG random value.
		register uint8_t slot; // The next slot in [0..63].

		do // Search for an open slot.
		{
			next = prng(state);
			slot = (uint8_t)((next) ? next % UINT64_C(32) : next); // Place it in the range.
		}
		while (used[slot]);

		used[slot] = true; // Mark it as used.

		register uint8_t byte = (slot > UINT8_C(3)) ? slot / UINT8_C(4) : UINT8_C(0); // Which byte.
		register uint8_t bit = (byte && slot) ? slot % UINT8_C(4) : UINT8_C(0); // Which bit in byte.

		data.byte[byte] = sm_disp_set_bit(data.byte[byte], bit, sm_disp_get_bit(value, i)); // Get it and set it.
	}

	return data.word;
}


// Coalesces the given uint32_t (value) back into a uint8_t according to a mapping function generated from the current state 
// of the PRNG, finally returning the uint8_t.
exported uint8_t callconv sm_disp_coalesce_32(uint32_t value, sm_get64_f rrng, sm_ran64_f prng, void* state)
{
	union { uint32_t word; uint8_t byte[4]; } data = { value }; // To gather the dispersed bits.
	bool used[32] = SM_DISP_FALSE_32; // Track used-up slots.
	register uint8_t result = UINT8_C(0);
	register uint8_t i;

	for (i = 0; i < UINT8_C(8); ++i) // Perform the coalescence.
	{
		register uint64_t next; // The next PRNG random value.
		register uint8_t slot; // The next slot in [0..63].
		register uint8_t byte = 0; // Which byte.
		register uint8_t bit = 0; // Which bit in byte.

		do // Search for an open slot.
		{
			next = prng(state);
			slot = (uint8_t)((next) ? next % UINT64_C(32) : next); // Place it in the range.
		}
		while (used[slot]);

		used[slot] = true; // Mark it as used.

		register uint8_t byte = (slot > UINT8_C(3)) ? slot / UINT8_C(4) : UINT8_C(0); // Which byte.
		register uint8_t bit = (byte && slot) ? slot % UINT8_C(4) : UINT8_C(0); // Which bit in byte.

		result = sm_disp_set_bit(result, i, sm_disp_get_bit(data.byte[byte], bit)); // Get it and set it.
	}

	return result;
}


#endif // INCLUDE_EXPAND_H



// bitfield.h - Declarations for manipulating variable-length bit-fields.


#ifndef INCLUDE_BITFIELD_H
#define INCLUDE_BITFIELD_H 1


#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "sm.h"


/*
	Bit-Field Schema / Meta-Data Layout

	Represents a byte-packed schema for selecting dispersed bits across a n-bit bit-field.

	<bit-field-schema> ::= <size> <checksum> <size> <bit-field-size> <bit-field-entries-count> (<bit-field-entry>+) ;
	<size> ::= uint64_t ; // Count of bytes in the overall structure, not including <size>.
	<checksum> ::= uint64_t ; // CRC-64 of the following data.
	<bit-field-size> ::= uint16_t ; // How many bytes in the actual bit-field.
	<bit-field-entries-count> ::= uint8_t ; // Count of <bit-field-entry>.
	<bit-field-entry> ::= <bit-index-count> (<bit-index>+) ; // A single schema entry.
	<bit-index-count> ::= uint8_t ; // Count of indices in this <bit-field-entry>, must be in [8, 16, 32, 64].
	<bit-index> :: uint16_t ; // Index of bit to select, unique across <bit-field-schema>. 
*/


#define SM_BIT_FIELD_ENTRY_MAX_INDICES       64 // Maximum possible count of indices in a bit field entry.
#define SM_BIT_FIELD_SCHEMA_MAX_ENTRIES      64 // Maximum possible count of bit field entries.
#define SM_BIT_FIELD_PERCENTAGE_RANDOM_BYTES 0.25F // Percentage of the bit field that may be composed of random bytes.
#define SM_BIT_FIELD_MAX_POSSIBLE_BITS       ((SM_BIT_FIELD_ENTRY_MAX_INDICES * 64U) + (unsigned)(SM_BIT_FIELD_ENTRY_MAX_INDICES * SM_BIT_FIELD_PERCENTAGE_RANDOM_BYTES)) // Maximum count of bits we may have in total.

// Unpacked bit-field schema entry structure.
typedef struct sm_bf_schema_entry_s
{
	uint8_t indices; // Count of indices, must be in [8, 16, 32, 64].
	uint16_t index[SM_BIT_FIELD_ENTRY_MAX_INDICES]; // Indexes of bits to select, unique across all entries in sm_bf_schema_t.
}
sm_bf_schema_entry_t;


// Unpacked bit-field schema structure.
typedef struct sm_bf_schema_s
{
	uint64_t checksum; // CRC-64 of the following data, not including 'checksum'.
	uint64_t size; // Count of bytes in this structure overall, not including 'checksum' and 'size'.
	uint16_t bytes; // How many bytes overall in the actual bit-field itself, including regions that may not be used.
	uint8_t entries; // Count of entries in 'entry'.
	sm_bf_schema_entry_t entry[SM_BIT_FIELD_SCHEMA_MAX_ENTRIES]; // Array of entries.
}
sm_bf_schema_t;


// Returns the truth-value of the bit at byte value[index].
inline static bool sm_bf_bit_get_byte(register uint8_t value, register uint8_t index)
{
	return ((value & (UINT8_C(1) << index)) != UINT8_C(0));
}


// Returns byte value with bit truth-value at value[index] set to state.
inline static uint8_t sm_bf_bit_set_byte(register uint8_t value, register uint8_t index, register bool state)
{
	return (state) ? (value | (UINT8_C(1) << index)) : (value & ~(UINT8_C(1) << index));
}


// Returns byte value with the bit at value[index] toggled.
inline static uint8_t sm_bf_bit_tog_byte(register uint8_t value, register uint8_t index)
{
	return (value ^ (UINT8_C(1) << index));
}


// Gets the bit truth value at index in field.
inline static bool sm_bf_bit_get(register uint8_t *restrict field, register uint16_t index)
{
	return (index > UINT16_C(7)) ? 
		sm_bf_bit_get_byte(field[index / UINT16_C(8)], (uint8_t)(index % UINT16_C(8))) :
		sm_bf_bit_get_byte(*field, (uint8_t)index);
}


// Sets the bit truth value at index in field to the specified value.
inline static void sm_bf_bit_set(register uint8_t *restrict field, uint16_t index, bool value)
{
	if (index > UINT16_C(7))
	{
		register const uint16_t byte = (index / UINT16_C(8));
		field[byte] = sm_bf_bit_set_byte(field[byte], index % UINT16_C(8), value);
	}
	else *field = sm_bf_bit_set_byte(*field, (uint8_t)index, value);
}


// Coalesces a uint8_t from field, according to the specified indices, where the count of elements at 
// indices must equal 8.
inline static uint8_t sm_bf_get_8(register uint8_t *restrict field, register uint16_t *restrict indices)
{
	register uint8_t result = UINT8_C(0);
	
	result = sm_bf_bit_set_byte(result, UINT8_C(0), sm_bf_bit_get(field, indices[UINT16_C(0)]));
	result = sm_bf_bit_set_byte(result, UINT8_C(1), sm_bf_bit_get(field, indices[UINT16_C(1)]));
	result = sm_bf_bit_set_byte(result, UINT8_C(2), sm_bf_bit_get(field, indices[UINT16_C(2)]));
	result = sm_bf_bit_set_byte(result, UINT8_C(3), sm_bf_bit_get(field, indices[UINT16_C(3)]));
	result = sm_bf_bit_set_byte(result, UINT8_C(4), sm_bf_bit_get(field, indices[UINT16_C(4)]));
	result = sm_bf_bit_set_byte(result, UINT8_C(5), sm_bf_bit_get(field, indices[UINT16_C(5)]));
	result = sm_bf_bit_set_byte(result, UINT8_C(6), sm_bf_bit_get(field, indices[UINT16_C(6)]));
	result = sm_bf_bit_set_byte(result, UINT8_C(7), sm_bf_bit_get(field, indices[UINT16_C(7)]));

	return result;
}


// Coalesces a uint16_t from field, according to the specified indices, where the count of elements at 
// indices must equal 16.
inline static uint16_t sm_bf_get_16(register uint8_t *restrict field, register uint16_t *restrict indices)
{
	register union { uint8_t b[2]; uint16_t v; } r;

	r.b[0] = sm_bf_get_8(field, indices + 0x00U);
	r.b[1] = sm_bf_get_8(field, indices + 0x08U);

	return r.v;
}


// Coalesces a uint32_t from field, according to the specified indices, where the count of elements at 
// indices must equal 32.
inline static uint32_t sm_bf_get_32(register uint8_t *restrict field, register uint16_t *restrict indices)
{
	register union { uint8_t b[4]; uint32_t v; } r;

	r.b[0] = sm_bf_get_8(field, indices + 0x00U);
	r.b[1] = sm_bf_get_8(field, indices + 0x08U);
	r.b[2] = sm_bf_get_8(field, indices + 0x10U);
	r.b[3] = sm_bf_get_8(field, indices + 0x18U);

	return r.v;
}


// Coalesces a uint64_t from field, according to the specified indices, where the count of elements at 
// indices must equal 64.
inline static uint64_t sm_bf_get_64(register uint8_t *restrict field, register uint16_t *restrict indices)
{
	register union { uint8_t b[8]; uint64_t v; } r;

	r.b[0] = sm_bf_get_8(field, indices + 0x00U);
	r.b[1] = sm_bf_get_8(field, indices + 0x08U);
	r.b[2] = sm_bf_get_8(field, indices + 0x10U);
	r.b[3] = sm_bf_get_8(field, indices + 0x18U);
	r.b[4] = sm_bf_get_8(field, indices + 0x20U);
	r.b[5] = sm_bf_get_8(field, indices + 0x28U);
	r.b[6] = sm_bf_get_8(field, indices + 0x30U);
	r.b[7] = sm_bf_get_8(field, indices + 0x38U);

	return r.v;
}


// Disperses an 8-bit value into field, according to the specified indices, where the count of elements at 
// indices must equal 8.
inline static void sm_bf_set_8(register uint8_t *restrict field, register uint16_t *restrict indices, register uint8_t value)
{
	sm_bf_bit_set(field, indices[0], sm_bf_bit_get_byte(value, UINT8_C(0)));
	sm_bf_bit_set(field, indices[1], sm_bf_bit_get_byte(value, UINT8_C(1)));
	sm_bf_bit_set(field, indices[2], sm_bf_bit_get_byte(value, UINT8_C(2)));
	sm_bf_bit_set(field, indices[3], sm_bf_bit_get_byte(value, UINT8_C(3)));
	sm_bf_bit_set(field, indices[4], sm_bf_bit_get_byte(value, UINT8_C(4)));
	sm_bf_bit_set(field, indices[5], sm_bf_bit_get_byte(value, UINT8_C(5)));
	sm_bf_bit_set(field, indices[6], sm_bf_bit_get_byte(value, UINT8_C(6)));
	sm_bf_bit_set(field, indices[7], sm_bf_bit_get_byte(value, UINT8_C(7)));
}


// Disperses a 16-bit value into field, according to the specified indices, where the count of elements at 
// indices must equal 16.
inline static void sm_bf_set_16(register uint8_t *restrict field, register uint16_t *restrict indices, uint16_t value)
{
	register union { uint16_t v; uint8_t b[2]; } s = { value };

	sm_bf_set_8(field, indices + 0x00U, s.b[0]);
	sm_bf_set_8(field, indices + 0x08U, s.b[1]);
}


// Disperses a 32-bit value into field, according to the specified indices, where the count of elements at 
// indices must equal 32.
inline static void sm_bf_set_32(register uint8_t *restrict field, register uint16_t *restrict indices, uint32_t value)
{
	register union { uint32_t v; uint8_t b[4]; } s = { value };

	sm_bf_set_8(field, indices + 0x00U, s.b[0]);
	sm_bf_set_8(field, indices + 0x08U, s.b[1]);
	sm_bf_set_8(field, indices + 0x10U, s.b[2]);
	sm_bf_set_8(field, indices + 0x18U, s.b[3]);
}


// Disperses a 64-bit value into field, according to the specified indices, where the count of elements at 
// indices must equal 64.
inline static void sm_bf_set_64(register uint8_t *restrict field, register uint16_t *restrict indices, uint64_t value)
{
	register union { uint64_t v; uint8_t b[8]; } s = { value };

	sm_bf_set_8(field, indices + 0x00U, s.b[0]);
	sm_bf_set_8(field, indices + 0x08U, s.b[1]);
	sm_bf_set_8(field, indices + 0x10U, s.b[2]);
	sm_bf_set_8(field, indices + 0x18U, s.b[3]);
	sm_bf_set_8(field, indices + 0x20U, s.b[4]);
	sm_bf_set_8(field, indices + 0x28U, s.b[5]);
	sm_bf_set_8(field, indices + 0x30U, s.b[6]);
	sm_bf_set_8(field, indices + 0x38U, s.b[7]);
}


// Coalesces bits from field into result, according to the specified schema. Result must have size sufficient to hold 
// all bits specified in schema.
inline static void sm_bf_read(register uint8_t *restrict field, register sm_bf_schema_t *restrict schema, uint8_t *restrict result)
{
	register uint8_t i, n = schema->entries;
	register uint8_t* chunk = result;

	for (i = UINT8_C(0); i < n; ++i)
	{
		if (schema->entry[i].indices == UINT8_C(0x08))
			*chunk = sm_bf_get_8(field, schema->entry[i].index);
		else if (schema->entry[i].indices == UINT8_C(0x10))
			*((uint16_t*)chunk) = sm_bf_get_16(field, schema->entry[i].index);
		else if (schema->entry[i].indices == UINT8_C(0x20))
			*((uint32_t*)chunk) = sm_bf_get_32(field, schema->entry[i].index);
		else *((uint64_t*)chunk) = sm_bf_get_64(field, schema->entry[i].index);
		chunk += (uint64_t)(schema->entry[i].indices / 8);
	}
}


// Disperses bits from buffer into field according to the specified schema. Field must have space sufficient to hold 
// all bits specified in schema.
inline static void sm_bf_write(register uint8_t *restrict field, register sm_bf_schema_t *restrict schema, uint8_t *restrict result)
{
	register uint8_t i, n = schema->entries;
	register uint8_t* chunk = result;

	for (i = UINT8_C(0); i < n; ++i)
	{
		if (schema->entry[i].indices == UINT8_C(0x08))
			sm_bf_set_8(field, schema->entry[i].index, *chunk);
		else if (schema->entry[i].indices == UINT8_C(0x10))
			sm_bf_set_16(field, schema->entry[i].index, *((uint16_t*)chunk));
		else if (schema->entry[i].indices == UINT8_C(0x20))
			sm_bf_set_32(field, schema->entry[i].index, *((uint32_t*)chunk));
		else sm_bf_set_64(field, schema->entry[i].index, *((uint64_t*)chunk));
		chunk += (uint64_t)(schema->entry[i].indices / 8);
	}
}


// Creates a new schema according to the element sizes in bits, specified in sizes, of count, given an entropy getter.
inline static bool sm_bf_create_schema(register uint8_t* sizes, register uint8_t count, sm_get64_f entropy, register sm_bf_schema_t *restrict schema)
{
	if (count > SM_BIT_FIELD_SCHEMA_MAX_ENTRIES)
		return false;

	register size_t i, m = (size_t)count;
	register size_t b = UINT64_C(0);

	// Compute how many bytes we need.

	for (i = UINT64_C(0); i < m; ++i)
	{
		const uint8_t s = sizes[i];
		if (s != UINT8_C(8) || s != UINT8_C(16) || s != UINT8_C(32) || s != UINT8_C(64)) // Constrain to valid sizes.
			return false;
		b += (size_t)(sizes[i] / UINT8_C(8)); // Add bytes.
	}

	register size_t k = (SM_BIT_FIELD_MAX_POSSIBLE_BITS / 8U);
	register size_t r = b + (UINT64_C(1) + (size_t)((((UINT64_C(1) + entropy()) % b) * SM_BIT_FIELD_PERCENTAGE_RANDOM_BYTES))); // Find a random amount of bytes < k.

	while (r > k)
		r = b + (UINT64_C(1) + (size_t)((((UINT64_C(1) + entropy()) % b) * SM_BIT_FIELD_PERCENTAGE_RANDOM_BYTES))); // Keep searching.

	schema->bytes = b = r; // Actual byte count.
	b *= UINT64_C(8); // Convert to bits.

	k = SM_BIT_FIELD_MAX_POSSIBLE_BITS;

	if (b > k) return false;

	register bool index[SM_BIT_FIELD_MAX_POSSIBLE_BITS]; // LUT for used-up indices.
	register bool* p = index;
	
	while (k-- > UINT64_C(0)) *p++ = false; // Zero it.

	for (i = UINT64_C(0); i < m; ++i) // For each new schema entry.
	{
		register size_t j, n = (size_t)(schema->entry[i].indices = sizes[i]);

		for (j = UINT64_C(0); j < n; ++j) // For each bit index.
		{
			uint16_t x = (uint16_t)((UINT64_C(1) + entropy()) % b); // Find a new unused index.
			while(index[x]) x = (uint16_t)((UINT64_C(1) + entropy()) % b); // Keep searching for a vacant slot.
			index[x] = true;
			schema->entry[i].index[j] = x;
		}
	}

	schema->entries = (uint8_t)m;

	return true;
}


#endif // INCLUDE_BITFIELD_H


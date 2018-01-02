// sm_internal.h


#include "config.h"
#include "sm.h"
#include "allocator.h"
#include "mutex.h"
#include "hash_table.h"


#ifndef INCLUDE_SM_INTERNAL_H
#define INCLUDE_SM_INTERNAL_H 1


// Random number generator entry.
typedef struct sm_rng_entry_s
{
	uint64_t state_size; // State vector size in bytes.
	sm_srs64_f seed_function; // Seed function pointer.
	uint64_t seed_size; // Seed function bytes.
	sm_ran64_f next_function; // Generate next function pointer.
	uint64_t next_size; // Next function bytes.
}
sm_rng_entry_t;


// The global context structure.
typedef struct sm_context_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.

	uint8_t padding_1[4];

	sm_mutex_t mutex; // Context/initialization mutex.
	
	uint8_t padding_2[8];

	sm_err_f error_handler; // Error handler.

	uint8_t padding_3[16];

	// Integral RNGs.

	uint8_t rng_count; // Count of RNG entries in tab.
	sm_rng_entry_t rng_tab[0xFF]; // Registered RNG entries.

	uint8_t padding_4[32];

	// Random support.

	sm_mutex_t rand_mutex; // Mutex for the random master.
	uint8_t rand_initialized; // Initialization flag.
	uint8_t rand_state[(sizeof(uint32_t) + (sizeof(uint64_t) * 16))]; // The  XorShift1024* state, if needed.
	uint8_t rand_rdrand; // RDRAND availability flag, 0xFF if not known yet, otherwise 0 or 1.
	sm_get64_f have_rdrand; // Test for RDRAND function.
	sm_get64_f next_rdrand; // Get next RDRAND function.

	uint8_t padding_5[64];

	// CRC support.

	void* crc_32_tab; // The 32-bit CRC LUT.
	sm_crc32_f crc_32_function; // The 32-bit CRC function.

	void* crc_64_tab; // The 64-bit CRC LUT.
	sm_crc64_f crc_64_function; // The 64-bit CRC function.

	uint8_t padding_6[128];

	// Memory management.

	sm_allocator_internal_t allocator; // Global allocator.

	uint8_t padding_7[256];

	sm_hash_table_t* keys; // Key store.
	sm_hash_table_t* data; // Data store.
	sm_hash_table_t* meta; // Meta store.

	uint8_t padding_8[512];
}
sm_context_t;


typedef struct sm_key_entry_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.
	uint8_t padding_1[8];
	uint64_t key; // Key.
	uint8_t padding_2[16];
	uint32_t check; // 32-bit CRC of key.
	uint8_t padding_3[32];
}
sm_key_entry_t;


typedef struct sm_data_entry_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.
	uint8_t padding_1[8];
	void* data;
	uint8_t padding_2[16];
}
sm_data_entry_t;


typedef struct sm_meta_entry_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.
	uint8_t padding_1[8];
	size_t bytes;
	uint8_t padding_2[16];
	size_t element;
	uint8_t padding_3[32];
	size_t count;
	uint8_t padding_4[64];
}
sm_meta_entry_t;


#endif // INCLUDE_SM_INTERNAL_H


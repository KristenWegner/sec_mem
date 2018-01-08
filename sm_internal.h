// sm_internal.h


#include <time.h>

#include "config.h"
#include "sm.h"
#include "allocator.h"
#include "mutex.h"
#include "hash_table.h"


#ifndef INCLUDE_SM_INTERNAL_H
#define INCLUDE_SM_INTERNAL_H 1


// Random number generator entry.
typedef halign(1) struct sm_rng_entry_s
{
	uint64_t state_size; // State vector size in bytes.
	sm_srs64_f seed_function; // Seed function pointer.
	uint64_t seed_size; // Seed function bytes.
	sm_ran64_f next_function; // Generate next function pointer.
	uint64_t next_size; // Next function bytes.
}
talign(1)
sm_rng_entry_t;


// The global context structure.
typedef halign(1) struct sm_context_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.

	// Mutex support.
	struct
	{
		sm_mutex_t lock; // Context/initialization mutex.
		sm_mutex_f create; // Create mutex.
		sm_mutex_f destroy; // Destroy mutex.
		sm_mutex_f enter; // Enter mutex.
		sm_mutex_f leave; // Leave mutex.
	}
	synchronization;

	sm_err_f error; // Error handler.

#if defined(SM_OS_WINDOWS)
	BOOL (__stdcall *protect)(LPVOID, SIZE_T, DWORD, PDWORD); // Used to make pages executable.
#else
	int (*protect)(void*, size_t, int); // Used to make pages executable.
#endif

	// Random support.
	struct
	{
		sm_mutex_t lock; // Mutex for the random master.
		uint8_t initialized; // Initialization flag.
		uint8_t state[(sizeof(uint32_t) + (sizeof(uint64_t) * 16))]; // The  XorShift1024* state, if needed.

		// RDRAND support.
		struct
		{
			uint8_t available; // RDRAND availability flag, 0xFF if not known yet, otherwise 0 or 1.
			sm_get64_f exists; // Test for RDRAND function.
			sm_get64_f next; // Get next RDRAND function.
		}
		rdrand;

		// Integral RNGs.
		struct
		{
			uint8_t count; // Count of RNG entries in tab.
			sm_rng_entry_t table[0xFF]; // Registered RNG entries.
		}
		integral;

		// Functions used for seeding entropy, if needed.
		struct
		{
			time_t (*get_time)(void*);
			int (*get_time_of_day)(void*, void*);
			clock_t (*get_clock)();
			pid_t (*get_process_id)();
			pid_t (*get_thread_id)();
			uid_t (*get_user_id)();
			uint64_t (*get_user_name_hash)();
#if defined(SM_OS_WINDOWS)
			uint64_t (*get_ticks)();
#endif
		}
		entropy;

		uint64_t (*method)(sm_t); // The RNG method.
	}
	random;

	// CRC support.
	struct
	{
		void* tab_32; // The 32-bit CRC LUT.
		sm_crc32_f crc_32; // The 32-bit CRC function.

		void* tab_64; // The 64-bit CRC LUT.
		sm_crc64_f crc_64; // The 64-bit CRC function.
	}
	checking;

	// Memory management.
	struct
	{
		sm_allocator_internal_t allocator; // Global allocator.

		void* (*allocate)(void*, size_t);
		void (*release)(void*, void*);
		void* (*resize)(void*, void*, size_t);
		void* (*resize_fixed)(void*, void*, size_t);
		void* (*align)(void*, size_t, size_t);
		size_t(*usable)(const void*);
		uint8_t (*trim)(void*, size_t);
		size_t (*footprint)(void*);

		sm_hash_table_t* keys; // Key store.
		sm_hash_table_t* data; // Data store.
		sm_hash_table_t* meta; // Meta store.
	}
	memory;
}
talign(1)
sm_context_t;


typedef halign(1) struct sm_key_entry_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.
	uint64_t key; // Key.
	uint32_t check; // 32-bit CRC of key.
}
talign(1)
sm_key_entry_t;


typedef halign(1) struct sm_data_entry_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.
	void* data;
	uint32_t check; // 32-bit CRC of data ptr.
}
talign(1)
sm_data_entry_t;


typedef halign(1) struct sm_meta_entry_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.
	size_t bytes;
	size_t element;
	size_t count;
}
talign(1)
sm_meta_entry_t;


#endif // INCLUDE_SM_INTERNAL_H


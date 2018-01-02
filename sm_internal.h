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

	uint8_t __padding_a[4];

	sm_mutex_t mutex; // Context/initialization mutex.
	
	uint8_t __padding_b[8];

	sm_err_f error; // Error handler.

	uint8_t __padding_c[16];

#if defined(SM_OS_WINDOWS)
	BOOL (__stdcall *protect)(LPVOID, SIZE_T, DWORD, PDWORD); // Used to make pages executable.
#else
	int (*protect)(void*, size_t, int); // Used to make pages executable.
#endif

	uint8_t __padding_d[32];

	// Random support.
	struct
	{
		sm_mutex_t mutex; // Mutex for the random master.
		uint8_t initialized; // Initialization flag.

		uint8_t __padding_e[64];

		uint8_t state[(sizeof(uint32_t) + (sizeof(uint64_t) * 16))]; // The  XorShift1024* state, if needed.

		uint8_t __padding_f[32];

		// RDRAND support.
		struct
		{
			uint8_t available; // RDRAND availability flag, 0xFF if not known yet, otherwise 0 or 1.
			sm_get64_f exists; // Test for RDRAND function.
			sm_get64_f next; // Get next RDRAND function.
		}
		rdrand;

		uint8_t __padding_g[16];

		// Integral RNGs.
		struct
		{
			uint8_t count; // Count of RNG entries in tab.
			sm_rng_entry_t table[0xFF]; // Registered RNG entries.
		}
		integral;

		uint8_t __padding_h[8];

		// Functions used for seeding entropy, if needed.
		struct
		{
			time_t (*get_tim)(void*);
			int (*get_tod)(void*, void*);
			clock_t (*get_clk)();
			pid_t (*get_pid)();
			pid_t (*get_tid)();
			uid_t (*get_uid)();
			uint64_t (*get_unh)();
#if defined(SM_OS_WINDOWS)
			uint64_t (*get_tik)();
#endif
		}
		entropy;

		void (*srand)(unsigned int);
		int (*rand)();

		uint8_t padding_i[4];
	}
	random;

	uint8_t __padding_j[8];

	// CRC support.
	struct
	{
		void* tab_32; // The 32-bit CRC LUT.
		sm_crc32_f crc_32; // The 32-bit CRC function.

		void* tab_64; // The 64-bit CRC LUT.
		sm_crc64_f crc_64; // The 64-bit CRC function.
	}
	checking;

	uint8_t __padding_k[16];

	// Memory management.
	struct
	{
		sm_allocator_internal_t allocator; // Global allocator.

		uint8_t __padding_l[32];

		sm_hash_table_t* keys; // Key store.
		sm_hash_table_t* data; // Data store.
		sm_hash_table_t* meta; // Meta store.
	}
	memory;

	uint8_t __padding_m[64];
}
sm_context_t;


typedef struct sm_key_entry_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.
	uint8_t __padding_a[4];
	uint64_t key; // Key.
	uint8_t __padding_b[8];
	uint32_t check; // 32-bit CRC of key.
	uint8_t __padding_c[4];
}
sm_key_entry_t;


typedef struct sm_data_entry_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.
	uint8_t __padding_a[4];
	void* data;
	uint8_t __padding_b[8];
	uint32_t check; // 32-bit CRC of data ptr.
	uint8_t __padding_c[4];
}
sm_data_entry_t;


typedef struct sm_meta_entry_s
{
	size_t size; // Size of this.
	uint8_t initialized; // Initialized flag.
	uint64_t crc; // 64-bit CRC of this.
	uint8_t __padding_a[4];
	size_t bytes;
	uint8_t __padding_b[8];
	size_t element;
	uint8_t __padding_c[16];
	size_t count;
	uint8_t __padding_d[8];
}
sm_meta_entry_t;


#endif // INCLUDE_SM_INTERNAL_H


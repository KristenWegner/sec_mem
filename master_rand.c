// master_rand.c - Master entropic/quasi-entropic random number generator.


#include <time.h>
#ifdef _DEBUG
#include <stdio.h>
#endif


#include "config.h"
#include "sm.h"
#include "mutex.h"
#include "bits.h"
#include "compatibility/gettimeofday.h"
#include "compatibility/getuid.h"


// XorShift1024* 64-bit seed, given state.
inline static void sm_master_rand_seed(void *restrict s, uint64_t seed)
{
	register uint32_t i, *p = s; *p = 0;
	register uint64_t z, *t = (uint64_t*)&p[1];

	for (i = 0; i < 0x10; ++i)
	{
		z = (seed += UINT64_C(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
		t[i] = z ^ (z >> 31);
	}
}


// XorShift1024* 64-bit generate next, given state.
inline static uint64_t sm_master_rand_next(void *restrict s)
{
	register uint32_t *p = s;
	register uint64_t *t = (uint64_t*)&p[1], s0 = t[*p], s1 = t[*p = (*p + 1) & 15];
	s1 ^= s1 << 31;
	t[*p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
	return t[*p] * UINT64_C(0x9E3779B97F4A7C13);
}


static sm_mutex_t sm_master_rand_mutex__; // Mutex for the random master.
static volatile bool sm_master_rand_initialized__ = false; // Initialization flag.
static uint8_t sm_master_rand_state__[(sizeof(uint32_t) + (sizeof(uint64_t) * 16))] = { 0 }; // The  XorShift1024* state, if needed.
static uint8_t sm_master_rand_rdrand__ = 0xFF; // RDRAND availability flag, 0xFF if not known yet, otherwise 0 or 1.
extern volatile sm_get64_f sm_op_have_rdrand_fn__;
extern volatile sm_get64_f sm_op_next_rdrand_fn__;


// Initializes the random master. If RDRAND is available, simply sets a flag, otherwise uses a variety of
// time, UID, PID, TID and other measures with bit twiddling to seed an XorShift1024* 64-bit state.
static void sm_master_rand_initialize()
{
	if (sm_master_rand_initialized__) return;

	sm_mutex_create(&sm_master_rand_mutex__); // Init the mutex.
	sm_mutex_lock(&sm_master_rand_mutex__); // Lock the master rand mutex.

	if (sm_master_rand_rdrand__ == 0xFF)
	{
		if (sm_op_have_rdrand_fn__ && sm_op_next_rdrand_fn__)
			sm_master_rand_rdrand__ = (sm_op_have_rdrand_fn__() != 0) ? 1 : 0; // Test for RDRAND.
		else sm_master_rand_rdrand__ = 0;
	}

	if (sm_master_rand_rdrand__) // Have RDRAND so no further init is needed.
	{
		sm_master_rand_initialized__ = true; // Set init flag.
		sm_mutex_unlock(&sm_master_rand_mutex__); // Unlock mutex.
		return;
	}

	// Do it the hard way.

	uint64_t rs = UINT64_C(18446744073709551557); // Largest 64-bit prime number.
	rs ^= sm_shuffle_64(time(NULL)); // XOR with unzipped time value.

#if defined(SM_OS_WINDOWS)
	rs ^= sm_yellow_64(GetTickCount64()); // XOR with yellow code of 64-bit tick count on Windows.
#endif

	struct timeval tv = { 0, 0 }; 

	if (!gettimeofday(&tv, NULL)) // Get time of day.
		rs ^= sm_yellow_64(sm_shuffle_64(tv.tv_sec) ^ sm_shuffle_64(tv.tv_usec)); // XOR with yellow code of unzipped XOR of time of day parts.

	rs ^= sm_unzip_64(sm_getpid()) ^ sm_shuffle_64(sm_gettid()) ^ sm_green_64(sm_getuid()) ^ sm_swap_64(sm_getunh()); // XOR with XOR of twiddled pid, tid, uid, and user name hash.
	rs = sm_yellow_64(rs); // Convert to yellow code.

	sm_master_rand_seed(sm_master_rand_state__, rs); // Actually seed the RNG.

	uint8_t ix, ns = 16 + ((rs ^ sm_master_rand_next(sm_master_rand_state__) + 1) % 32); // Get a random count of times up to 16 + [0 .. 32].

	for (ix = 0; ix < ns; ++ix) 
		sm_master_rand_next(sm_master_rand_state__); // Warm it up.

	sm_master_rand_initialized__ = true; // Set init flag.
	sm_mutex_unlock(&sm_master_rand_mutex__); // Unlock random master mutex.
}


// Generates a new 64-bit entropic or quasi-entropic value.
exported uint64_t callconv sm_master_rand()
{
	sm_master_rand_initialize(); // Initialize if needed.

	if (sm_master_rand_rdrand__ == 1 && sm_op_next_rdrand_fn__) // Have RDRAND, so just return the next value.
		return sm_op_next_rdrand_fn__();

	// Do it the hard way.

	sm_mutex_lock(&sm_master_rand_mutex__); // Lock the master rand mutex.

	uint64_t rv = sm_master_rand_next(sm_master_rand_state__); // Get the next random value to return.

	// Reseed indicator.
	uint64_t rs = sm_yellow_64(sm_shuffle_64(time(NULL))) ^ sm_master_rand_next(sm_master_rand_state__); // The yellow of the shuffled time and XOR of another random value.

	if (!rs || (rs % 8) == 0) // Every zero or zero modulus 8 of rs, do a re-seed.
	{
#ifdef _DEBUG
		printf("RE-SEED\n");
#endif

		struct timeval tv = { 0, 0 };

		if (!gettimeofday(&tv, NULL))
		{
			if ((rs ^ ~tv.tv_usec) & 1) // Mix it up a bit.
				rv ^= sm_green_64(sm_red_64(tv.tv_sec) ^ sm_shuffle_64(tv.tv_usec));
			else rv ^= sm_yellow_64(sm_shuffle_64(tv.tv_sec) ^ sm_red_64(tv.tv_usec));
		}

		rv ^= sm_shuffle_64(clock()); // XOR with shuffled clock.

#if defined(SM_OS_WINDOWS)
		rs ^= sm_yellow_64(sm_shuffle_64(GetTickCount64())); // XOR with yellow shuffle of 64-bit tick count on Windows.
#endif

		rs = sm_yellow_64(rv ^ ~rs ^ sm_master_rand_next(sm_master_rand_state__));

		if (rv & 1) // Cyclic shift a variable amount [1 .. 63] bits left or right.
			rs = sm_rotl_64(rs, 1 + ((1 + rv) % 62));
		else rs = sm_rotl_64(rs, 1 + ((1 + rv) % 62));

		// Now re-seed.

		sm_master_rand_seed(sm_master_rand_state__, rs);

		register uint8_t ix, ns = 16 + ((sm_master_rand_next(sm_master_rand_state__) + 1) % 16); // Get a count of re-warm-up steps.

		for (ix = 0; ix < ns; ++ix) // Re-warm it up a random count of times.
			sm_master_rand_next(sm_master_rand_state__);

		rv = sm_master_rand_next(sm_master_rand_state__); // Get the result value.
	}

	sm_mutex_unlock(&sm_master_rand_mutex__); // Unlock random master mutex.

	return rv;
}



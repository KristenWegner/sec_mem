// sm_random.c - Master entropic/quasi-entropic random number generator.


#include <time.h>
#ifdef _DEBUG
#include <stdio.h>
#endif


#include "config.h"
#include "sm.h"
#include "sm_internal.h"
#include "mutex.h"
#include "bits.h"
#include "compatibility/gettimeofday.h"


// XorShift1024* 64-bit seed, given state.
inline static void sm_random_seed(void *restrict s, uint64_t seed)
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
inline static uint64_t sm_random_next(void *restrict s)
{
	register uint32_t *p = s;
	register uint64_t *t = (uint64_t*)&p[1], s0 = t[*p], s1 = t[*p = (*p + 1) & 15];
	s1 ^= s1 << 31;
	t[*p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
	return t[*p] * UINT64_C(0x9E3779B97F4A7C13);
}


// Initializes the random master. If RDRAND is available, simply sets a flag, otherwise uses a variety of
// time, UID, PID, TID and other measures with bit twiddling to seed an XorShift1024* 64-bit state.
inline static void sm_random_initialize(sm_context_t* context)
{
	if (!context) return;

	context->synchronization.enter(&context->random.lock); // Lock the master rand mutex.

	if (context->random.rdrand.available == 0xFF)
	{
		if (context->random.rdrand.exists && context->random.rdrand.next)
			context->random.rdrand.available = (context->random.rdrand.exists() != 0) ? 1 : 0; // Test for RDRAND.
		else context->random.rdrand.available = 0;
	}

	if (context->random.rdrand.available) // Have RDRAND so no further init is needed.
	{
		context->random.initialized = 1; // Set init flag.
		context->synchronization.leave(&context->random.lock); // Unlock mutex.
		return;
	}

	// Do it the hard way.

	uint64_t rs = UINT64_C(0x23AD3C18F); // Arbitrary prime.
	rs ^= sm_shuffle_64(context->random.entropy.get_time(NULL)); // XOR with unzipped time value.

#if defined(SM_OS_WINDOWS)
	rs ^= sm_yellow_64(context->random.entropy.get_ticks()); // XOR with yellow code of 64-bit tick count on Windows.
#endif

	struct timeval tv = { 0, 0 }; 

	if (!context->random.entropy.get_time_of_day(&tv, NULL)) // Get time of day.
		rs ^= sm_yellow_64(sm_shuffle_64(tv.tv_sec) ^ sm_shuffle_64(tv.tv_usec)); // XOR with yellow code of shuffled XOR of time of day parts.

	rs ^= sm_unzip_64(context->random.entropy.get_process_id()) ^ sm_shuffle_64(context->random.entropy.get_thread_id()) ^ sm_green_64(context->random.entropy.get_user_id()) ^ sm_swap_64(context->random.entropy.get_user_name_hash()); // XOR with XOR of twiddled pid, tid, uid, and user name hash.
	rs = sm_yellow_64(rs); // Convert to yellow code.

	sm_random_seed(context->random.state, rs); // Actually seed the RNG.

	uint8_t ix, ns = 16 + ((rs ^ sm_random_next(context->random.state) + 1) % 32); // Get a random count of times up to 16 + [0 .. 32].

	for (ix = 0; ix < ns; ++ix) 
		(void)sm_random_next(context->random.state); // Warm it up.

	context->random.initialized = 1; // Set init flag.
	context->synchronization.leave(&context->random.lock); // Unlock random master mutex.
}


// Default RNG using CRT rand.
inline static uint64_t sm_default_rand(register int (*rf)())
{
	register uint8_t i, n = 1 + (1 + rf()) % 16;
	register union { uint32_t i[2]; uint64_t q; } u1, u2;

	u1.i[0] = (uint32_t)rf();
	u1.i[1] = (uint32_t)rf();

	for (i = 0; i < n; ++i)
	{
		if (i & 1) u1.i[0] ^= (uint32_t)rf();
		else u1.i[1] ^= (uint32_t)rf();
		u1.q = sm_rotl_64(u1.q, i + 1);
	}

	u2.i[0] = (uint32_t)rf();
	u2.i[1] = (uint32_t)rf();

	return sm_shuffle_64(u1.q) ^ sm_yellow_64(sm_shuffle_64(u2.q));
}


// Generates a new 64-bit entropic or quasi-entropic value.
exported uint64_t callconv sm_random(sm_t sm)
{
	if (!sm) return sm_default_rand(rand);

	sm_context_t* context = (sm_context_t*)sm;

	if (!context->random.initialized)
		sm_random_initialize(context); // Initialize if needed.

	if (context->random.rdrand.available == 1 && context->random.rdrand.next) // Have RDRAND, so just return the next value.
		return context->random.rdrand.next();

	// Do it the hard way.

	context->synchronization.enter(&context->random.lock); // Lock the master rand mutex.

	uint64_t rv = sm_random_next(context->random.state); // Get the next random value to return.

	// Reseed indicator.
	uint64_t rs = sm_yellow_64(sm_shuffle_64(context->random.entropy.get_time(NULL))) ^ sm_random_next(context->random.state); // The yellow of the shuffled time and XOR of another random value.

	if (!sm || !rs || (rs % 16) == 0) // Every zero or zero modulus 8 of rs, do a re-seed.
	{
		rv ^= sm_shuffle_64(context->random.entropy.get_clock()); // XOR with shuffled clock.

#if defined(SM_OS_WINDOWS)
		rs ^= sm_yellow_64(sm_shuffle_64(context->random.entropy.get_ticks())); // XOR with yellow shuffle of 64-bit tick count on Windows.
#else
		struct timeval tv = { 0, 0 };
		if (!context->random.entropy.get_time of day(&tv, NULL))
		{
			if ((rs ^ ~tv.tv_usec) & 1) // Mix it up a bit.
				rv ^= sm_green_64(sm_green_64(tv.tv_sec) ^ sm_shuffle_64(tv.tv_usec));
			else rv ^= sm_yellow_64(sm_shuffle_64(tv.tv_sec) ^ sm_green_64(tv.tv_usec));
		}
#endif

		rs = sm_yellow_64(rv ^ ~rs ^ sm_random_next(context->random.state));

		if (rv & 1) // Cyclic shift a variable amount [1 .. 63] bits left or right.
			rs = sm_rotl_64(rs, 1 + ((1 + rv) % 62));
		else rs = sm_rotr_64(rs, 1 + ((1 + rv) % 62));

		// Now re-seed.
		sm_random_seed(context->random.state, rs);

		register uint8_t ix, ns = 16 + ((sm_random_next(context->random.state) + 1) % 16); // Get a count of re-warm-up steps.

		for (ix = 0; ix < ns; ++ix) // Re-warm it up a random count of times.
			sm_random_next(context->random.state);

		rv = sm_random_next(context->random.state); // Get the result value.
	}

	context->synchronization.leave(&context->random.lock); // Unlock random master mutex.

	return rv;
}



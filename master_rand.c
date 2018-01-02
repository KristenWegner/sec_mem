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
static void sm_random_initialize(sm_context_t* context)
{
	if (context->rand_initialized) return;

#ifdef _DEBUG
	fprintf(stderr, "(sm_random: initialize: enter)\n");
#endif

	sm_mutex_create(&context->rand_mutex); // Init the mutex.
	sm_mutex_lock(&context->rand_mutex); // Lock the master rand mutex.

	if (context->rand_rdrand == 0xFF)
	{
		if (context->have_rdrand && context->next_rdrand)
			context->rand_rdrand = (context->have_rdrand() != 0) ? 1 : 0; // Test for RDRAND.
		else context->rand_rdrand = 0;
#ifdef _DEBUG
		fprintf(stderr, "(sm_random: have rdrand: %d)\n", context->rand_rdrand);
#endif
	}

	if (context->rand_rdrand) // Have RDRAND so no further init is needed.
	{
		context->rand_initialized = 1; // Set init flag.
		sm_mutex_unlock(&context->rand_mutex); // Unlock mutex.

#ifdef _DEBUG
		fprintf(stderr, "(sm_random: initialize: leave)\n");
#endif

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

#ifdef _DEBUG
	fprintf(stderr, "(sm_random: initialize: seed = 0x%" PRIX64 ")\n", rs);
#endif

	sm_master_rand_seed(context->rand_state, rs); // Actually seed the RNG.

	uint8_t ix, ns = 16 + ((rs ^ sm_random_next(context->rand_state) + 1) % 32); // Get a random count of times up to 16 + [0 .. 32].

	for (ix = 0; ix < ns; ++ix) 
		sm_random_next(context->rand_state); // Warm it up.

	context->rand_initialized = 1; // Set init flag.
	sm_mutex_unlock(&context->rand_mutex); // Unlock random master mutex.

#ifdef _DEBUG
	fprintf(stderr, "(sm_random: initialize: leave)\n");
#endif
}


// Generates a new 64-bit entropic or quasi-entropic value.
exported uint64_t callconv sm_random(sm_t sm)
{
	static volatile uint8_t srand_called__ = 0;

	if (!sm)
	{
#ifdef _DEBUG
		fprintf(stderr, "(sm_random: no context)\n");
#endif
		if (!srand_called__)
		{
			struct timeval tv = { 0, 0 };
			uint32_t sr = (uint32_t)time(NULL);
			if (!gettimeofday(&tv, NULL)) // Get time of day.
				sr ^= (uint32_t)sm_yellow_64(sm_shuffle_64(tv.tv_sec) ^ sm_shuffle_64(tv.tv_usec)); // XOR with yellow code of unzipped XOR of time of day parts.
			sr ^= (uint32_t)sm_unzip_64(sm_getpid()) ^ sm_shuffle_64(sm_gettid()) ^ sm_green_64(sm_getuid()) ^ sm_swap_64(sm_getunh()); // XOR with XOR of twiddled pid, tid, uid, and user name hash.
			sr = (uint32_t)sm_yellow_64(sr); // Convert to yellow code.
#ifdef _DEBUG
			fprintf(stderr, "(sm_random: no context, seed = 0x%08X)\n", sr);
#endif
			srand(sr);
			srand_called__ = 1;
		}

		uint64_t rr = rand();
		rr <<= 32;
		return sm_yellow_64(sm_shuffle_64(rr | rand()));
	}

	sm_context_t* context = (sm_context_t*)sm;
	void* state = &context->rand_state[0];

	sm_random_initialize(sm); // Initialize if needed.

	if (context->rand_rdrand == 1 && context->next_rdrand) // Have RDRAND, so just return the next value.
		return context->next_rdrand();

	// Do it the hard way.

	sm_mutex_lock(&context->rand_mutex); // Lock the master rand mutex.

	uint64_t rv = sm_random_next(state); // Get the next random value to return.

	// Reseed indicator.
	uint64_t rs = sm_yellow_64(sm_shuffle_64(time(NULL))) ^ sm_random_next(state); // The yellow of the shuffled time and XOR of another random value.

	if (!sm || !rs || (rs % 16) == 0) // Every zero or zero modulus 8 of rs, do a re-seed.
	{
#ifdef _DEBUG
		fprintf(stderr, "(sm_random: re-seed: enter)\n");
#endif

		rv ^= sm_shuffle_64(clock()); // XOR with shuffled clock.

#if defined(SM_OS_WINDOWS)
		rs ^= sm_yellow_64(sm_shuffle_64(GetTickCount64())); // XOR with yellow shuffle of 64-bit tick count on Windows.
#else
		struct timeval tv = { 0, 0 };

		if (!gettimeofday(&tv, NULL)) // XOR with time of day.
		{
			if ((rs ^ ~tv.tv_usec) & 1) // Mix it up a bit.
				rv ^= sm_green_64(sm_green_64(tv.tv_sec) ^ sm_shuffle_64(tv.tv_usec));
			else rv ^= sm_yellow_64(sm_shuffle_64(tv.tv_sec) ^ sm_green_64(tv.tv_usec));
		}
#endif

		rs = sm_yellow_64(rv ^ ~rs ^ sm_random_next(state));

		if (rv & 1) // Cyclic shift a variable amount [1 .. 63] bits left or right.
			rs = sm_rotl_64(rs, 1 + ((1 + rv) % 62));
		else rs = sm_rotl_64(rs, 1 + ((1 + rv) % 62));

		// Now re-seed.

#ifdef _DEBUG
		fprintf(stderr, "(sm_random: re-seed: seed = 0x%" PRIX64 ")\n", rs);
#endif

		sm_master_rand_seed(state, rs);

		register uint8_t ix, ns = 16 + ((sm_random_next(state) + 1) % 16); // Get a count of re-warm-up steps.

		for (ix = 0; ix < ns; ++ix) // Re-warm it up a random count of times.
			sm_random_next(state);

		rv = sm_random_next(state); // Get the result value.

#ifdef _DEBUG
		fprintf(stderr, "(sm_random: re-seed: leave)\n");
#endif
	}

	sm_mutex_unlock(&context->rand_mutex); // Unlock random master mutex.

	return rv;
}



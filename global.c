// global.c - Secure memory manager globals.


#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <intrin.h>

#include "config.h"
#include "hashing.h"
#include "random.h"



static void* _0 = 0; // Pointer to the root of all things.






/*
static bool random_global_initialized__ = false; // Global random state initialization flag.
static random_state random_global_state__ = { { 0 }, 0 }; // Global random state.


// Initializes the global random number generator.
void random_global_initialize(uint64_t seed)
{
	if (random_global_initialized__) return;
	if (seed == 0) seed = random_generate_seed();
	random_initialize(&random_global_state__, seed);
}


// Gets the next random uint64_t value from the global random number generator.
uint64_t random_next_integer(void)
{
	return random_integer(&random_global_state__);
}


// Gets the next random byte value from the global random number generator.
uint8_t random_next_byte(void)
{
	return random_byte(&random_global_state__);
}
*/



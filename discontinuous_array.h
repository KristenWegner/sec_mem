// discontinuous_array.h - Represents an array where each element is separated by any number of non-used bytes.


#include "config.h"
#include "mutex.h"


#ifndef INCLUDE_DISCONTINUOUS_ARRAY_H
#define INCLUDE_DISCONTINUOUS_ARRAY_H 1


// Represents an array where each element is separated by any number of non-used bytes.
typedef struct sec_discontinuous_array
{
	// Current seed.
	uint64_t seed;

	// Random state.
	uint8_t state[(sizeof(int32_t) + (sizeof(uint64_t) * 16))];

	// Maximal count of spacing bytes.
	size_t spacing;

	// Size of a single element in bytes.
	size_t element;

	// Count of elements.
	size_t count;

	// The actual data.
	uint8_t* data;

	// Current index, for iteration.
	uint64_t index;

	// Current pointer, for iteration.
	void* pointer;

	// Mutex.
	mutex_t mutex;
}
sec_discontinuous_array;





#endif // INCLUDE_DISCONTINUOUS_ARRAY_H




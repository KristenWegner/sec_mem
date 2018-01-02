// hash_table.c - Hash table implementation.


#ifndef INCLUDE_HASH_TABLE_H
#define INCLUDE_HASH_TABLE_H 1


#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "config.h"
#include "mutex.h"
#include "allocator.h"


// Result codes.

#define SM_RC_NO_ERROR					0
#define SM_RC_OBJECT_NULL				1
#define SM_RC_ARGUMENT_NULL				2
#define SM_RC_ALLOCATION_FAILED			3
#define SM_RC_OPERATION_BLOCKED			4
#define SM_RC_INTERNAL_REFERENCE_NULL	5
#define SM_RC_NOT_FOUND					6


// Return code type.
typedef uint16_t sm_rc; 


// Represents an iterator to a the hash table.
typedef uint64_t sec_hash_table_iterator;

// A callback function type for computing 32-bit hashes of elements. Returns the hash code. Null values must be handled internally.
typedef uint64_t (*sec_hash_table_hasher)(const void* data, size_t size);

// A callback function type for visiting elements in a hash table. Receives the iterator, the key, it's size, a pointer to the stored 
// data in *data, and a user-defined context. Returns true to continue iterating.
typedef bool (*sec_hash_table_visitor)(sec_hash_table_iterator iterator, void* key, size_t size, void** data, void* context);


// Represents a general-purpose hash table.
typedef struct sm_hash_table_s 
{
	// Reference to the memory allocator.
	void* allocator;

	// The count of buckets.
	uint64_t buckets;

	// The count of items in the table.
	uint64_t count;

	// The count of buckets that are occupied.
	uint64_t occupied;

	// The upper bound on capacity.
	uint64_t upper;

	// Hash table lookup flags.
	uint32_t* flags;

	// The default data hasher.
	sec_hash_table_hasher hasher;

	// The size of a key.
	size_t size;

	// Vector of pointers to keys.
	void** keys;

	// Vector of pointers to values.
	void** values;

	// Object mutex.
	sm_mutex_t mutex;
}
sm_hash_table_t;


// Methods


// Creates a new hash table with the given key size, hash function. Result is in *object. Returns status.
sm_rc sec_hash_table_create(sm_allocator_internal_t allocator, sm_hash_table_t** object, size_t size, sec_hash_table_hasher hasher);

// Destroys the given hash table in *object. Returns status.
sm_rc sec_hash_table_destroy(sm_hash_table_t** object);

// Clears the given hash table without deallocating memory. Returns status.
sm_rc sec_hash_table_clear(sm_hash_table_t *restrict object);

// Resizes the specified hash table with the given count of buckets. Returns status.
sm_rc sec_hash_table_resize(sm_hash_table_t *restrict object, uint64_t buckets);

// Retrieves an element by key from the given hash table. Result is an iterator to the found element, or sec_hash_table_iterate_end(object) 
// if the element is absent. Returns status.
sm_rc sec_hash_table_find(sm_hash_table_t *restrict object, void* key, sec_hash_table_iterator* result);

// Inserts a new element into the hash table with the given key and value. Result is an iterator to the inserted element. Returns status.
sm_rc sec_hash_table_insert(sm_hash_table_t *restrict object, void* key, void* value, sec_hash_table_iterator* result);

// Removes the specified entry from the hash table. Returns status.
sm_rc sec_hash_table_remove_at(sm_hash_table_t *restrict object, sec_hash_table_iterator iterator);

// Tests whether the bucket at the given address contains data. Result is in *result. Returns status.
sm_rc sec_hash_table_exists_at(sm_hash_table_t *restrict object, sec_hash_table_iterator iterator, bool* result);

// Gets the key at the given iterator position. Result is in *result. Returns status.
sm_rc sec_hash_table_get_key(sm_hash_table_t *restrict object, sec_hash_table_iterator iterator, void** result);

// Gets the value at the given iterator position. Result is in *result. Returns status.
sm_rc sec_hash_table_get_value(sm_hash_table_t *restrict object, sec_hash_table_iterator iterator, void** result);

// Tests whether the given key corresponds to any data. Result is in *result. Returns status.
sm_rc sec_hash_table_contains(sm_hash_table_t *restrict object, void* key, bool* result);

// Inserts a new element into the hash table with the given key and value. Returns status.
sm_rc sec_hash_table_set(sm_hash_table_t *restrict object, void* key, void* value);

// Gets the value corresponding to the given key. Result is in *result. Returns status.
sm_rc sec_hash_table_get(sm_hash_table_t *restrict object, void* key, void** result);

// Removes the entry corresponding to the specified key from the hash table. Returns status.
sm_rc sec_hash_table_remove(sm_hash_table_t *restrict object, void* key);

// Gets the begin iterator to the specified hash table. Result is in *result. Returns status.
sm_rc sec_hash_table_iterate_begin(sm_hash_table_t *restrict object, sec_hash_table_iterator* result);

// Gets the end iterator to the specified hash table. Result is in *result. Returns status.
sm_rc sec_hash_table_iterate_end(sm_hash_table_t *restrict object, sec_hash_table_iterator* result);

// Gets the count of elements in the hash table. Result is in *result. Returns status.
sm_rc sec_hash_table_count(sm_hash_table_t *restrict object, uint64_t* result);

// Gets the count of buckets in the hash table. Result is in *result. Returns status.
sm_rc sec_hash_table_buckets(sm_hash_table_t *restrict object, uint64_t* result);

// Iterates over the specified hash table using the given visitor callback.
sm_rc sec_hash_table_iterate(sm_hash_table_t *restrict object, sec_hash_table_visitor visitor, void* context);

// Default data hasher.
uint64_t sec_hash_table_default_hasher(const void* data, size_t size);


#endif // INCLUDE_HASH_TABLE_H


// hash_table.c - Hash table implementation.


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <intrin.h>
#include <limits.h>

#include "sm.h"
#include "hash_table.h"
#include "mutex.h"
#include "allocator.h"


extern uint64_t callconv sm_random();


// Methods


sm_rc sec_hash_table_create(sm_allocator_internal_t allocator, sm_hash_table_t** object, size_t size, sec_hash_table_hasher hasher)
{
	sm_rc rc;
	sm_hash_table_t* temp;

	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (hasher == NULL) return SM_RC_ARGUMENT_NULL;

	*object = NULL;

	temp = (sm_hash_table_t*)sm_space_allocate(allocator, sizeof(sm_hash_table_t));

	if (temp == NULL) return SM_RC_ALLOCATION_FAILED;

	register uint8_t* p = (uint8_t*)temp;
	register size_t n = sizeof(sm_hash_table_t);
	while (n-- > 0U) *p++ = 0;

	temp->allocator = allocator;
	temp->hasher = hasher;
	temp->size = size;

	if (!sm_mutex_create(&temp->mutex))
	{
		p = (uint8_t*)temp;
		n = sizeof(sm_hash_table_t);
		while (n-- > 0U) *p++ = (uint8_t)sm_random(NULL);

		sm_space_free(allocator, temp);

		return SM_RC_ALLOCATION_FAILED;
	}

	*object = temp;

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_destroy(sm_hash_table_t** object)
{
	sm_hash_table_t* temp;

	if (object == NULL || *object == NULL) return SM_RC_OBJECT_NULL;

	temp = *object;

	if (!sm_mutex_lock(&temp->mutex)) return SM_RC_OPERATION_BLOCKED;

	*object = NULL;

	sm_allocator_internal_t allocator = temp->allocator;

	sm_space_free(allocator, temp->keys);
	temp->keys = NULL;

	sm_space_free(allocator, temp->flags);
	temp->flags = NULL;

	sm_space_free(allocator, temp->values);
	temp->values = NULL;

	sm_mutex_unlock(&temp->mutex);
	sm_mutex_destroy(&temp->mutex);

	register uint8_t* p = (uint8_t*)temp;
	register size_t n = sizeof(sm_hash_table_t);
	while (n-- > 0U) *p++ = (uint8_t)sm_random(NULL);

	sm_space_free(allocator, temp);

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_clear(sm_hash_table_t *restrict object)
{
	uint64_t buckets;

	if (object == NULL) return SM_RC_OBJECT_NULL;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->flags == NULL)
	{
		sm_mutex_unlock(&object->mutex);

		return SM_RC_INTERNAL_REFERENCE_NULL;
	}

	buckets = object->buckets;

	register uint8_t* p = (uint8_t*)object->flags;
	register size_t n = (buckets < 16ULL ? 1ULL : buckets >> 4ULL) * sizeof(uint64_t);
	while (n-- > 0U) *p++ = 0xAA;

	object->count = object->occupied = 0;
	
	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_find(sm_hash_table_t *restrict object, void* key, sec_hash_table_iterator* result)
{
	uint64_t k, i, last, mask, step = 0;

	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (result == NULL) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->buckets)
	{
		mask = object->buckets - 1;

		k = object->hasher(key, object->size);

		i = k & mask;
		last = i;

		while (!((object->flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 2ULL) && (((object->flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 1ULL) || (object->keys[i] != key)))
		{
			i = (i + (++step)) & mask;

			if (i == last)
			{
				*result = object->buckets;

				sm_mutex_unlock(&object->mutex);

				return SM_RC_NO_ERROR;
			}
		}

		*result = ((object->flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 3ULL) ? object->buckets : i;

		sm_mutex_unlock(&object->mutex);

		return SM_RC_NO_ERROR;
	}
	
	sm_mutex_unlock(&object->mutex);

	return SM_RC_NOT_FOUND;
}


static bool sec_hash_table_resize__(sm_hash_table_t *restrict object, uint64_t buckets);


sm_rc sec_hash_table_resize(sm_hash_table_t *restrict object, uint64_t buckets)
{
	if (object == NULL) return SM_RC_OBJECT_NULL;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (!sec_hash_table_resize__(object, buckets))
	{
		sm_mutex_unlock(&object->mutex);

		return SM_RC_ALLOCATION_FAILED;
	}

	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_contains(sm_hash_table_t *restrict object, void* key, bool* result)
{
	sm_rc rc;
	sec_hash_table_iterator it = 0;
	if ((rc = sec_hash_table_find(object, key, &it)) != SM_RC_NO_ERROR) return rc;
	return sec_hash_table_exists_at(object, it, result);
}


sm_rc sec_hash_table_set(sm_hash_table_t *restrict object, void* key, void* value)
{
	sec_hash_table_iterator it = 0;
	return sec_hash_table_insert(object, key, value, &it);
}


sm_rc sec_hash_table_get(sm_hash_table_t *restrict object, void* key, void** result)
{
	sm_rc rc;
	sec_hash_table_iterator it = 0;
	if ((rc = sec_hash_table_find(object, key, &it)) != SM_RC_NO_ERROR) return rc;
	return sec_hash_table_get_value(object, it, result);
}


sm_rc sec_hash_table_remove(sm_hash_table_t *restrict object, void* key)
{
	sm_rc rc;
	sec_hash_table_iterator it = 0;
	if ((rc = sec_hash_table_find(object, key, &it)) != SM_RC_NO_ERROR) return rc;
	return sec_hash_table_remove_at(object, it);
}


sm_rc sec_hash_table_insert(sm_hash_table_t *restrict object, void* key, void* value, sec_hash_table_iterator* result)
{
	sm_rc rc;
	uint64_t x, k, i, site, last, mask, step;

	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (key == NULL || result == NULL) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->occupied >= object->upper)
	{
		if (object->buckets > (object->count << 1)) // Update.
		{
			if (!sec_hash_table_resize__(object, object->buckets - 1))
			{
				*result = object->buckets;

				sm_mutex_unlock(&object->mutex);

				return SM_RC_ALLOCATION_FAILED;
			}
		}
		else if (!sec_hash_table_resize__(object, object->buckets + 1)) // Expand.
		{
			*result = object->buckets;

			sm_mutex_unlock(&object->mutex);

			return SM_RC_ALLOCATION_FAILED;
		}
	}

	step = 0;
	mask = object->buckets - 1;
	x = site = object->buckets;
	k = object->hasher(key, object->size);
	i = k & mask;

	if (((object->flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 2ULL))
		x = i;
	else
	{
		last = i;

		while (!((object->flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 2ULL) && (((object->flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 1ULL) || (object->keys[i] != key)))
		{
			if (((object->flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 1ULL))
				site = i;

			i = (i + (++step)) & mask;

			if (i == last) 
			{ 
				x = site;

				break; 
			}
		}
		if (x == object->buckets)
		{
			if (((object->flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 2ULL) && site != object->buckets)
				x = site;
			else x = i;
		}
	}

	if (((object->flags[x >> 4ULL] >> ((x & 15ULL) << 1ULL)) & 2ULL)) // Not present.
	{
		object->keys[x] = key;
		object->values[x] = value;
		object->flags[x >> 4ULL] &= ~(3ULL << ((x & 15ULL) << 1ULL));
		object->count++; 
		object->occupied++;

		rc = SM_RC_NO_ERROR;
	}
	else if (((object->flags[x >> 4ULL] >> ((x & 15ULL) << 1ULL)) & 1ULL)) // Deleted.
	{
		object->keys[x] = key;
		object->values[x] = value;
		object->flags[x >> 4ULL] &= ~(3ULL << ((x & 15ULL) << 1ULL));
		object->count++;

		rc = SM_RC_NO_ERROR;
	}
	else rc = SM_RC_NO_ERROR;

	*result = x;

	sm_mutex_unlock(&object->mutex);

	return rc;
}


sm_rc sec_hash_table_remove_at(sm_hash_table_t *restrict object, sec_hash_table_iterator iterator)
{
	if (object == NULL) return SM_RC_OBJECT_NULL;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (iterator != object->buckets && !((object->flags[iterator >> 4ULL] >> ((iterator & 15ULL) << 1ULL)) & 3ULL))
	{
		object->flags[iterator >> 4ULL] |= 1ULL << ((iterator & 15ULL) << 1ULL);
		object->count--;
	}

	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_exists_at(sm_hash_table_t *restrict object, sec_hash_table_iterator iterator, bool* result)
{
	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (result == NULL) return SM_RC_ARGUMENT_NULL;

	*result = false;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	*result = !((object->flags[iterator >> 4ULL] >> ((iterator & 0xFULL) << 1ULL)) & 3ULL);

	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_get_key(sm_hash_table_t *restrict object, sec_hash_table_iterator iterator, void** result)
{
	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (result == NULL) return SM_RC_ARGUMENT_NULL;

	*result = NULL;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->keys == NULL || object->values == NULL)
	{
		sm_mutex_unlock(&object->mutex);

		return SM_RC_INTERNAL_REFERENCE_NULL;
	}

	*result = object->keys[iterator];

	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_get_value(sm_hash_table_t *restrict object, sec_hash_table_iterator iterator, void** result)
{
	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (result == NULL) return SM_RC_ARGUMENT_NULL;

	*result = NULL;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->keys == NULL || object->values == NULL)
	{
		sm_mutex_unlock(&object->mutex);

		return SM_RC_INTERNAL_REFERENCE_NULL;
	}

	*result = object->values[iterator];

	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}

sm_rc sec_hash_table_iterate_begin(sm_hash_table_t *restrict object, sec_hash_table_iterator* result)
{
	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (result == NULL) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->keys == NULL || object->values == NULL)
	{
		sm_mutex_unlock(&object->mutex);

		return SM_RC_INTERNAL_REFERENCE_NULL;
	}

	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_iterate_end(sm_hash_table_t *restrict object, sec_hash_table_iterator* result)
{
	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (result == NULL) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	*result = (sec_hash_table_iterator)object->buckets;

	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_count(sm_hash_table_t *restrict object, uint64_t* result)
{
	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (result == NULL) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	*result = object->count;

	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_buckets(sm_hash_table_t *restrict object, uint64_t* result)
{
	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (result == NULL) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	*result = object->buckets;

	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}


sm_rc sec_hash_table_iterate(sm_hash_table_t *restrict object, sec_hash_table_visitor visitor, void* context)
{
	register uint64_t i, n;

	if (object == NULL) return SM_RC_OBJECT_NULL;
	if (visitor == NULL) return SM_RC_ARGUMENT_NULL;

	if (!sm_mutex_lock(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->flags == NULL || object->keys == NULL || object->values == NULL)
	{
		sm_mutex_unlock(&object->mutex);

		return SM_RC_INTERNAL_REFERENCE_NULL;
	}

	n = object->buckets;

	for (i = 0; i != n; ++i)
	{
		if ((object->flags[i >> 4ULL] >> ((i & 0xFULL) << 1ULL)) & 3ULL) 
			continue;

		if (!visitor((sec_hash_table_iterator)i, object->keys[i], object->size, &(object->values[i]), context))
			break;
	}

	sm_mutex_unlock(&object->mutex);

	return SM_RC_NO_ERROR;
}


uint64_t sec_hash_table_default_hasher(const void* data, size_t size)
{
	register size_t i;
	uint64_t h = 5381ULL;
	const uint8_t* d = (uint8_t*)data;

	if (d == NULL) return 0;

	for (i = 0; i < size; ++i)
		h = (h << 5ULL) - h + (uint64_t)d[i];

	return h;
}


// Internals


static bool sec_hash_table_resize__(sm_hash_table_t *restrict object, uint64_t buckets)
{
	static const double upper = 0.77;

	void *key, *val, *tmp;
	void **keys, **vals, **ppt;
	uint32_t* flags = NULL;
	uint64_t i, j = 1, k, nnb, mask, step = 0;

	--buckets;
	buckets |= buckets >> 1ULL;
	buckets |= buckets >> 2ULL;
	buckets |= buckets >> 4ULL;
	buckets |= buckets >> 8ULL;
	buckets |= buckets >> 16ULL;
	++buckets;

	if (buckets < 4) buckets = 4;

	if (object->count >= (uint64_t)(buckets * upper + 0.5))
		j = 0;
	else
	{
		nnb = (buckets < 16 ? 1 : buckets >> 4ULL);

		flags = (uint32_t*)sm_space_allocate(object->allocator, nnb * sizeof(uint32_t));

		if (flags == NULL) return false;

		register uint8_t* p = (uint8_t*)flags;
		register size_t n = nnb * sizeof(uint32_t);
		while (n-- > 0U) *p++ = 0xAA;

		if (object->buckets < buckets)
		{
			keys = (void**)sm_space_realloc(object->allocator, object->keys, buckets * sizeof(void*));

			if (keys == NULL)
			{
				free(flags);

				return false;
			}

			object->keys = keys;

			vals = (void**)sm_space_realloc(object->allocator, object->values, buckets * sizeof(void*));

			if (vals == NULL)
			{
				sm_space_free(object->allocator, flags);

				return false;
			}

			object->values = vals;
		}
	}

	if (j) // Re-hash.
	{
		for (j = 0; j != object->buckets; ++j)
		{
			if (((object->flags[j >> 4ULL] >> ((j & 15ULL) << 1ULL)) & 3ULL) == 0)
			{
				key = object->keys[j];
				mask = buckets - 1;
				val = object->values[j];

				object->flags[j >> 4ULL] |= 1ULL << ((j & 15ULL) << 1ULL);

				while (true)
				{
					step = 0;

					k = object->hasher(key, object->size);

					i = k & mask;

					while (!((flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 2ULL))
						i = (i + (++step)) & mask;

					flags[i >> 4ULL] &= ~(2ULL << ((i & 15ULL) << 1ULL));

					if (i < object->buckets && ((object->flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 3ULL) == 0)
					{
						tmp = object->keys[i];
						object->keys[i] = key;
						key = tmp;

						tmp = object->values[i];
						object->values[i] = val;
						val = tmp;

						object->flags[i >> 4ULL] |= 1ULL << ((i & 15ULL) << 1ULL);
					}
					else
					{
						object->keys[i] = key;
						object->values[i] = val;

						break;
					}
				}
			}
		}

		if (object->buckets > buckets) // Shrink.
		{
			ppt = (void**)sm_space_realloc(object->allocator, object->keys, buckets * sizeof(void*));

			if (ppt == NULL) return false;

			object->keys = ppt;

			ppt = (void**)sm_space_realloc(object->allocator, object->values, buckets * sizeof(void*));

			if (ppt == NULL) return false;

			object->values = ppt;
		}

		sm_space_free(object->allocator, object->flags);

		object->flags = flags;
		object->buckets = buckets;
		object->occupied = object->count;
		object->upper = (uint64_t)(object->buckets * upper + 0.5);
	}

	return true;
}


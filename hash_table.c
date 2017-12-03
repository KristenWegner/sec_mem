// hash_table.c - Hash table implementation.


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <intrin.h>
#include <limits.h>

#include "secure_memory.h"
#include "hash_table.h"


// Methods


sec_rc sec_hash_table_create(sec_hash_table** object, size_t size, sec_hash_table_hasher hasher, bool mutex)
{
	sec_rc rc;
	sec_hash_table* temp;

	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (hasher == NULL) return SEC_RC_ARGUMENT_NULL;

	*object = NULL;

	temp = (sec_hash_table*)malloc(sizeof(sec_hash_table));

	if (temp == NULL) return SEC_RC_ALLOCATION_FAILED;

	memset(temp, 0, sizeof(sec_hash_table));

	temp->hasher = hasher;
	temp->size = size;

	if (mutex)
	{
		if ((rc = sb_mutex_create(&(temp->mutex))) != SEC_RC_NO_ERROR)
		{
			free(temp);
			return rc;
		}
	}
	else temp->mutex = NULL;

	*object = temp;

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_destroy(sec_hash_table** object)
{
	sec_hash_table* temp;

	if (object == NULL || *object == NULL) return SEC_RC_OBJECT_NULL;

	temp = *object;

	if (!sb_lockx__(temp)) return SEC_RC_OPERATION_BLOCKED;

	*object = NULL;

	free(temp->keys);
	temp->keys = NULL;

	free(temp->flags);
	temp->flags = NULL;

	free(temp->values);
	temp->values = NULL;

	if (temp->mutex != NULL)
	{
		sb_mutex_unlock_final(temp->mutex);
		sb_mutex_destroy(&(temp->mutex));
	}

	free(temp);

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_clear(sec_hash_table *restrict object)
{
	uint64_t buckets;

	if (object == NULL) return SEC_RC_OBJECT_NULL;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	if (object->flags == NULL)
	{
		sb_unlockx__(object);

		return SEC_RC_INTERNAL_REFERENCE_NULL;
	}

	buckets = object->buckets;

	memset(object->flags, 0xAA, (buckets < 16ULL ? 1ULL : buckets >> 4ULL) * sizeof(uint64_t));

	object->count = object->occupied = 0;
	
	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_find(sec_hash_table *restrict object, void* key, sec_hash_table_iterator* result)
{
	uint64_t k, i, last, mask, step = 0;

	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (result == NULL) return SEC_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

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

				sb_unlockx__(object);

				return SEC_RC_NO_ERROR;
			}
		}

		*result = ((object->flags[i >> 4ULL] >> ((i & 15ULL) << 1ULL)) & 3ULL) ? object->buckets : i;

		sb_unlockx__(object);

		return SEC_RC_NO_ERROR;
	}
	
	sb_unlockx__(object);

	return SEC_RC_NOT_FOUND;
}


static bool sec_hash_table_resize__(sec_hash_table *restrict object, uint64_t buckets);


sec_rc sec_hash_table_resize(sec_hash_table *restrict object, uint64_t buckets)
{
	if (object == NULL) return SEC_RC_OBJECT_NULL;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	if (!sec_hash_table_resize__(object, buckets))
	{
		sb_unlockx__(object);

		return SEC_RC_ALLOCATION_FAILED;
	}

	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_contains(sec_hash_table *restrict object, void* key, bool* result)
{
	sec_rc rc;
	sec_hash_table_iterator it = 0;
	if ((rc = sec_hash_table_find(object, key, &it)) != SEC_RC_NO_ERROR) return rc;
	return sec_hash_table_exists_at(object, it, result);
}


sec_rc sec_hash_table_set(sec_hash_table *restrict object, void* key, void* value)
{
	sec_hash_table_iterator it = 0;
	return sec_hash_table_insert(object, key, value, &it);
}


sec_rc sec_hash_table_get(sec_hash_table *restrict object, void* key, void** result)
{
	sec_rc rc;
	sec_hash_table_iterator it = 0;
	if ((rc = sec_hash_table_find(object, key, &it)) != SEC_RC_NO_ERROR) return rc;
	return sec_hash_table_get_value(object, it, result);
}


sec_rc sec_hash_table_remove(sec_hash_table *restrict object, void* key)
{
	sec_rc rc;
	sec_hash_table_iterator it = 0;
	if ((rc = sec_hash_table_find(object, key, &it)) != SEC_RC_NO_ERROR) return rc;
	return sec_hash_table_remove_at(object, it);
}


sec_rc sec_hash_table_insert(sec_hash_table *restrict object, void* key, void* value, sec_hash_table_iterator* result)
{
	sec_rc rc;
	uint64_t x, k, i, site, last, mask, step;

	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (key == NULL || result == NULL) return SEC_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	if (object->occupied >= object->upper)
	{
		if (object->buckets > (object->count << 1)) // Update.
		{
			if (!sec_hash_table_resize__(object, object->buckets - 1))
			{
				*result = object->buckets;

				sb_unlockx__(object);

				return SEC_RC_ALLOCATION_FAILED;
			}
		}
		else if (!sec_hash_table_resize__(object, object->buckets + 1)) // Expand.
		{
			*result = object->buckets;

			sb_unlockx__(object);

			return SEC_RC_ALLOCATION_FAILED;
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

		rc = SEC_RC_NO_ERROR;
	}
	else if (((object->flags[x >> 4ULL] >> ((x & 15ULL) << 1ULL)) & 1ULL)) // Deleted.
	{
		object->keys[x] = key;
		object->values[x] = value;
		object->flags[x >> 4ULL] &= ~(3ULL << ((x & 15ULL) << 1ULL));
		object->count++;

		rc = SEC_RC_NO_ERROR;
	}
	else rc = SEC_RC_NO_ERROR;

	*result = x;

	sb_unlockx__(object);

	return rc;
}


sec_rc sec_hash_table_remove_at(sec_hash_table *restrict object, sec_hash_table_iterator iterator)
{
	if (object == NULL) return SEC_RC_OBJECT_NULL;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	if (iterator != object->buckets && !((object->flags[iterator >> 4ULL] >> ((iterator & 15ULL) << 1ULL)) & 3ULL))
	{
		object->flags[iterator >> 4ULL] |= 1ULL << ((iterator & 15ULL) << 1ULL);
		object->count--;
	}

	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_exists_at(sec_hash_table *restrict object, sec_hash_table_iterator iterator, bool* result)
{
	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (result == NULL) return SEC_RC_ARGUMENT_NULL;

	*result = false;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	*result = !((object->flags[iterator >> 4ULL] >> ((iterator & 0xFULL) << 1ULL)) & 3ULL);

	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_get_key(sec_hash_table *restrict object, sec_hash_table_iterator iterator, void** result)
{
	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (result == NULL) return SEC_RC_ARGUMENT_NULL;

	*result = NULL;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	if (object->keys == NULL || object->values == NULL)
	{
		sb_unlockx__(object);

		return SEC_RC_INTERNAL_REFERENCE_NULL;
	}

	*result = object->keys[iterator];

	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_get_value(sec_hash_table *restrict object, sec_hash_table_iterator iterator, void** result)
{
	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (result == NULL) return SEC_RC_ARGUMENT_NULL;

	*result = NULL;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	if (object->keys == NULL || object->values == NULL)
	{
		sb_unlockx__(object);

		return SEC_RC_INTERNAL_REFERENCE_NULL;
	}

	*result = object->values[iterator];

	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
}

sec_rc sec_hash_table_iterate_begin(sec_hash_table *restrict object, sec_hash_table_iterator* result)
{
	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (result == NULL) return SEC_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	if (object->keys == NULL || object->values == NULL)
	{
		sb_unlockx__(object);

		return SEC_RC_INTERNAL_REFERENCE_NULL;
	}

	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_iterate_end(sec_hash_table *restrict object, sec_hash_table_iterator* result)
{
	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (result == NULL) return SEC_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	*result = (sec_hash_table_iterator)object->buckets;

	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_count(sec_hash_table *restrict object, uint64_t* result)
{
	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (result == NULL) return SEC_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	*result = object->count;

	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_buckets(sec_hash_table *restrict object, uint64_t* result)
{
	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (result == NULL) return SEC_RC_ARGUMENT_NULL;

	*result = 0;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	*result = object->buckets;

	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
}


sec_rc sec_hash_table_iterate(sec_hash_table *restrict object, sec_hash_table_visitor visitor, void* context)
{
	register uint64_t i, n;

	if (object == NULL) return SEC_RC_OBJECT_NULL;
	if (visitor == NULL) return SEC_RC_ARGUMENT_NULL;

	if (!sb_lockx__(object)) return SEC_RC_OPERATION_BLOCKED;

	if (object->flags == NULL || object->keys == NULL || object->values == NULL)
	{
		sb_unlockx__(object);

		return SEC_RC_INTERNAL_REFERENCE_NULL;
	}

	n = object->buckets;

	for (i = 0; i != n; ++i)
	{
		if ((object->flags[i >> 4ULL] >> ((i & 0xFULL) << 1ULL)) & 3ULL) 
			continue;

		if (!visitor((sec_hash_table_iterator)i, object->keys[i], object->size, &(object->values[i]), context))
			break;
	}

	sb_unlockx__(object);

	return SEC_RC_NO_ERROR;
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


static bool sec_hash_table_resize__(sec_hash_table *restrict object, uint64_t buckets)
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

		flags = (uint32_t*)malloc(nnb * sizeof(uint32_t));

		if (flags == NULL) return false;

		memset(flags, 0xAA, nnb * sizeof(uint32_t));

		if (object->buckets < buckets)
		{
			keys = (void**)realloc(object->keys, buckets * sizeof(void*));

			if (keys == NULL)
			{
				free(flags);

				return false;
			}

			object->keys = keys;

			vals = (void**)realloc(object->values, buckets * sizeof(void*));

			if (vals == NULL)
			{
				free(flags);

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
			ppt = (void**)realloc(object->keys, buckets * sizeof(void*));

			if (ppt == NULL) return false;

			object->keys = ppt;

			ppt = (void**)realloc(object->values, buckets * sizeof(void*));

			if (ppt == NULL) return false;

			object->values = ppt;
		}

		free(object->flags);

		object->flags = flags;
		object->buckets = buckets;
		object->occupied = object->count;
		object->upper = (uint64_t)(object->buckets * upper + 0.5);
	}

	return true;
}


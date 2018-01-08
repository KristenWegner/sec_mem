// tab.c - Hash table implementation.


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "../sm.h"
#include "../sm_internal.h"
#include "tab.h"


// Methods


exported sm_rc callconv tab_create(sm_t sm, sm_tab_t** object, size_t size, sm_hsh64_f hasher)
{
	sm_tab_t* temp;

	if (!sm) return SM_RC_OBJECT_NULL;
	if (!object) return SM_RC_OBJECT_NULL;
	if (!hasher) return SM_RC_ARGUMENT_NULL;

	*object = NULL;

	sm_context_t* context = (sm_context_t*)sm;

	temp = (sm_tab_t*)context->memory.allocate(context->memory.allocator, sizeof(sm_tab_t));

	if (temp == NULL) return SM_RC_ALLOCATION_FAILED;

	register uint8_t* p = (uint8_t*)temp;
	register size_t n = sizeof(sm_tab_t);
	while (n-- > UINT64_C(0)) *p++ = 0;

	temp->size = sizeof(sm_tab_t);
	temp->initialized = 1;
	temp->crc = UINT64_C(0);
	temp->context = context;
	temp->hasher = hasher;
	temp->key = size;

	if (!context->synchronization.create(&temp->mutex))
	{
		p = (uint8_t*)temp;
		n = sizeof(sm_tab_t);
		while (n-- > UINT64_C(0)) *p++ = (uint8_t)context->random.method(context);

		context->memory.release(context->memory.allocator, temp);

		return SM_RC_ALLOCATION_FAILED;
	}

	*object = temp;

	return SM_RC_NO_ERROR;
}


exported sm_rc callconv tab_destroy(sm_tab_t** object)
{
	sm_tab_t* temp;

	if (!object || !*object) return SM_RC_OBJECT_NULL;

	temp = *object;

	sm_context_t* context = temp->context;

	if (!context->synchronization.enter(&temp->mutex)) 
		return SM_RC_OPERATION_BLOCKED;

	*object = NULL;

	context->memory.release(context->memory.allocator, temp->keys);
	temp->keys = NULL;

	context->memory.release(context->memory.allocator, temp->flags);
	temp->flags = NULL;

	context->memory.release(context->memory.allocator, temp->values);
	temp->values = NULL;

	context->synchronization.leave(&temp->mutex);
	context->synchronization.destroy(&temp->mutex);

	register uint8_t* p = (uint8_t*)temp;
	register size_t n = sizeof(sm_tab_t);
	while (n-- > UINT64_C(0)) *p++ = (uint8_t)context->random.method(context);

	context->memory.release(context->memory.allocator, temp);

	return SM_RC_NO_ERROR;
}


exported sm_rc callconv tab_clear(sm_tab_t *restrict object)
{
	uint64_t buckets;

	if (!object) return SM_RC_OBJECT_NULL;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->flags == NULL)
	{
		context->synchronization.leave(&object->mutex);

		return SM_RC_INTERNAL_REFERENCE_NULL;
	}

	buckets = object->buckets;

	register uint8_t* p = (uint8_t*)object->flags;
	register size_t n = (buckets < UINT64_C(16) ? UINT64_C(1) : buckets >> 4) * sizeof(uint64_t);
	while (n-- > UINT64_C(0)) *p++ = 0xAAU;

	object->count = object->occupied = UINT64_C(0);
	
	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


inline static sm_rc tab_find__(sm_tab_t *restrict object, void* key, sm_tab_iterator_t* result)
{
	uint64_t k, i, last, mask, step = UINT64_C(0);

	if (!object) return SM_RC_OBJECT_NULL;
	if (!result) return SM_RC_ARGUMENT_NULL;

	*result = UINT64_C(0);

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->buckets)
	{
		mask = object->buckets - UINT64_C(1);

		k = object->hasher(key, object->key);

		i = k & mask;
		last = i;

		while (!((object->flags[i >> 4] >> ((i & 15U) << 1)) & 2U) && (((object->flags[i >> 4] >> ((i & 15U) << 1)) & 1U) || (object->keys[i] != key)))
		{
			i = (i + (++step)) & mask;

			if (i == last)
			{
				*result = object->buckets;

				context->synchronization.leave(&object->mutex);

				return SM_RC_NO_ERROR;
			}
		}

		*result = ((object->flags[i >> 4] >> ((i & 15U) << 1)) & 3U) ? object->buckets : i;

		context->synchronization.leave(&object->mutex);

		return SM_RC_NO_ERROR;
	}

	context->synchronization.leave(&object->mutex);

	return SM_RC_NOT_FOUND;
}


exported sm_rc callconv tab_find(sm_tab_t *restrict object, void* key, sm_tab_iterator_t* result)
{
	return tab_find__(object, key, result);
}


inline static uint8_t tab_resize__(sm_tab_t *restrict object, uint64_t buckets)
{
	static const long double upper = 0.77L;

	void *key, *val, *tmp;
	void **keys, **vals, **ppt;
	uint32_t* flags = NULL;
	uint64_t i, j = 1, k, nnb, mask, step = UINT64_C(0);

	--buckets;
	buckets |= buckets >> 1;
	buckets |= buckets >> 2;
	buckets |= buckets >> 4;
	buckets |= buckets >> 8;
	buckets |= buckets >> 16;
	++buckets;

	sm_context_t* context = object->context;

	if (buckets < UINT64_C(4)) buckets = UINT64_C(4);

	if (object->count >= (uint64_t)(buckets * upper + 0.5L))
		j = UINT64_C(0);
	else
	{
		nnb = (buckets < UINT64_C(16) ? UINT64_C(1) : buckets >> 4);

		flags = (uint32_t*)context->memory.allocate(context->memory.allocator, nnb * sizeof(uint32_t));

		if (flags == NULL) return 0;

		register uint8_t* p = (uint8_t*)flags;
		register size_t n = nnb * sizeof(uint32_t);
		while (n-- > UINT64_C(0)) *p++ = 0xAAU;

		if (object->buckets < buckets)
		{
			keys = (void**)context->memory.resize(context->memory.allocator, object->keys, buckets * sizeof(void*));

			if (keys == NULL)
			{
				context->memory.release(context->memory.allocator, flags);

				return 0;
			}

			object->keys = keys;

			vals = (void**)context->memory.resize(context->memory.allocator, object->values, buckets * sizeof(void*));

			if (vals == NULL)
			{
				context->memory.release(context->memory.allocator, flags);

				return 0;
			}

			object->values = vals;
		}
	}

	if (j) // Re-hash.
	{
		for (j = 0; j != object->buckets; ++j)
		{
			if (((object->flags[j >> 4] >> ((j & 15U) << 1)) & 3U) == 0)
			{
				key = object->keys[j];
				mask = buckets - 1;
				val = object->values[j];

				object->flags[j >> 4] |= 1U << ((j & 15U) << 1);

				while (1)
				{
					step = 0;

					k = object->hasher(key, object->key);

					i = k & mask;

					while (!((flags[i >> 4] >> ((i & 15U) << 1)) & 2U))
						i = (i + (++step)) & mask;

					flags[i >> 4] &= ~(2U << ((i & 15U) << 1));

					if (i < object->buckets && ((object->flags[i >> 4] >> ((i & 15U) << 1)) & 3U) == 0)
					{
						tmp = object->keys[i];
						object->keys[i] = key;
						key = tmp;

						tmp = object->values[i];
						object->values[i] = val;
						val = tmp;

						object->flags[i >> 4] |= 1U << ((i & 15U) << 1);
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
			ppt = (void**)context->memory.resize(context->memory.allocator, object->keys, buckets * sizeof(void*));

			if (ppt == NULL) return 0;

			object->keys = ppt;

			ppt = (void**)context->memory.resize(context->memory.allocator, object->values, buckets * sizeof(void*));

			if (ppt == NULL) return 0;

			object->values = ppt;
		}

		context->memory.release(context->memory.allocator, object->flags);

		object->flags = flags;
		object->buckets = buckets;
		object->occupied = object->count;
		object->upper = (uint64_t)(object->buckets * upper + 0.5L);
	}

	return 1;
}


exported sm_rc callconv tab_resize(sm_tab_t *restrict object, uint64_t buckets)
{
	if (!object) return SM_RC_OBJECT_NULL;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (!tab_resize__(object, buckets))
	{
		context->synchronization.leave(&object->mutex);

		return SM_RC_ALLOCATION_FAILED;
	}

	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


inline static sm_rc tab_exists_at__(sm_tab_t *restrict object, sm_tab_iterator_t iterator, uint8_t* result)
{
	if (!object) return SM_RC_OBJECT_NULL;
	if (!result) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	*result = !((object->flags[iterator >> 4] >> ((iterator & 0xFU) << 1)) & 3U);

	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


inline static sm_rc tab_insert__(sm_tab_t *restrict object, void* key, void* value, sm_tab_iterator_t* result)
{
	sm_rc rc;
	uint64_t x, k, i, site, last, mask, step;

	if (!object) return SM_RC_OBJECT_NULL;
	if (!key || !result) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->occupied >= object->upper)
	{
		if (object->buckets > (object->count << 1)) // Update.
		{
			if (!tab_resize__(object, object->buckets - 1))
			{
				*result = object->buckets;

				context->synchronization.leave(&object->mutex);

				return SM_RC_ALLOCATION_FAILED;
			}
		}
		else if (!tab_resize__(object, object->buckets + 1)) // Expand.
		{
			*result = object->buckets;

			context->synchronization.leave(&object->mutex);

			return SM_RC_ALLOCATION_FAILED;
		}
	}

	step = 0;
	mask = object->buckets - 1;
	x = site = object->buckets;
	k = object->hasher(key, object->key);
	i = k & mask;

	if (((object->flags[i >> 4] >> ((i & 15U) << 1)) & 2U))
		x = i;
	else
	{
		last = i;

		while (!((object->flags[i >> 4] >> ((i & 15U) << 1)) & 2U) && (((object->flags[i >> 4] >> ((i & 15U) << 1)) & 1U) || (object->keys[i] != key)))
		{
			if (((object->flags[i >> 4] >> ((i & 15U) << 1)) & 1U))
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
			if (((object->flags[i >> 4] >> ((i & 15U) << 1)) & 2U) && site != object->buckets)
				x = site;
			else x = i;
		}
	}

	if (((object->flags[x >> 4] >> ((x & 15U) << 1)) & 2U)) // Not present.
	{
		object->keys[x] = key;
		object->values[x] = value;
		object->flags[x >> 4] &= ~(3U << ((x & 15U) << 1));
		object->count++;
		object->occupied++;

		rc = SM_RC_NO_ERROR;
	}
	else if (((object->flags[x >> 4] >> ((x & 15U) << 1)) & 1U)) // Deleted.
	{
		object->keys[x] = key;
		object->values[x] = value;
		object->flags[x >> 4] &= ~(3U << ((x & 15U) << 1));
		object->count++;

		rc = SM_RC_NO_ERROR;
	}
	else rc = SM_RC_NO_ERROR;

	*result = x;

	context->synchronization.leave(&object->mutex);

	return rc;
}


inline static sm_rc tab_remove_at__(sm_tab_t *restrict object, sm_tab_iterator_t iterator)
{
	if (!object) return SM_RC_OBJECT_NULL;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (iterator != object->buckets && !((object->flags[iterator >> 4] >> ((iterator & 15U) << 1)) & 3U))
	{
		object->flags[iterator >> 4] |= 1U << ((iterator & 15U) << 1);
		object->count--;
	}

	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


exported sm_rc callconv tab_contains(sm_tab_t *restrict object, void* key, uint8_t* result)
{
	sm_rc rc;
	sm_tab_iterator_t it = 0;
	if ((rc = tab_find__(object, key, &it)) != SM_RC_NO_ERROR) return rc;
	return tab_exists_at__(object, it, result);
}


exported sm_rc callconv tab_set(sm_tab_t *restrict object, void* key, void* value)
{
	sm_tab_iterator_t it = 0;
	return tab_insert__(object, key, value, &it);
}


inline static sm_rc tab_get_value__(sm_tab_t *restrict object, sm_tab_iterator_t iterator, void** result)
{
	if (!object) return SM_RC_OBJECT_NULL;
	if (!result) return SM_RC_ARGUMENT_NULL;

	*result = NULL;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->keys == NULL || object->values == NULL)
	{
		context->synchronization.leave(&object->mutex);

		return SM_RC_INTERNAL_REFERENCE_NULL;
	}

	*result = object->values[iterator];

	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


exported sm_rc callconv tab_get(sm_tab_t *restrict object, void* key, void** result)
{
	sm_rc rc;
	sm_tab_iterator_t it = 0;
	if ((rc = tab_find__(object, key, &it)) != SM_RC_NO_ERROR) return rc;
	return tab_get_value__(object, it, result);
}


exported sm_rc callconv tab_remove(sm_tab_t *restrict object, void* key)
{
	sm_rc rc;
	sm_tab_iterator_t it = 0;
	if ((rc = tab_find__(object, key, &it)) != SM_RC_NO_ERROR) return rc;
	return tab_remove_at__(object, it);
}


exported sm_rc callconv tab_insert(sm_tab_t *restrict object, void* key, void* value, sm_tab_iterator_t* result)
{
	return tab_insert__(object, key, value, result);
}


exported sm_rc callconv tab_remove_at(sm_tab_t *restrict object, sm_tab_iterator_t iterator)
{
	return tab_remove_at__(object, iterator);
}


exported sm_rc callconv tab_exists_at(sm_tab_t *restrict object, sm_tab_iterator_t iterator, uint8_t* result)
{
	if (!object) return SM_RC_OBJECT_NULL;
	if (!result) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	*result = !((object->flags[iterator >> 4] >> ((iterator & 0xFULL) << 1)) & 3U);

	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


exported sm_rc callconv tab_get_key(sm_tab_t *restrict object, sm_tab_iterator_t iterator, void** result)
{
	if (!object) return SM_RC_OBJECT_NULL;
	if (!result) return SM_RC_ARGUMENT_NULL;

	*result = NULL;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->keys == NULL || object->values == NULL)
	{
		context->synchronization.leave(&object->mutex);

		return SM_RC_INTERNAL_REFERENCE_NULL;
	}

	*result = object->keys[iterator];

	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


exported sm_rc callconv tab_get_value(sm_tab_t *restrict object, sm_tab_iterator_t iterator, void** result)
{
	return tab_get_value__(object, iterator, result);
}


exported sm_rc callconv tab_iterate_begin(sm_tab_t *restrict object, sm_tab_iterator_t* result)
{
	if (!object) return SM_RC_OBJECT_NULL;
	if (!result) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->keys == NULL || object->values == NULL)
	{
		context->synchronization.leave(&object->mutex);

		return SM_RC_INTERNAL_REFERENCE_NULL;
	}

	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


exported sm_rc callconv tab_iterate_end(sm_tab_t *restrict object, sm_tab_iterator_t* result)
{
	if (!object) return SM_RC_OBJECT_NULL;
	if (!result) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	*result = (sm_tab_iterator_t)object->buckets;

	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


exported sm_rc callconv tab_count(sm_tab_t *restrict object, uint64_t* result)
{
	if (!object) return SM_RC_OBJECT_NULL;
	if (!result) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	*result = object->count;

	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


exported sm_rc callconv tab_buckets(sm_tab_t *restrict object, uint64_t* result)
{
	if (!object) return SM_RC_OBJECT_NULL;
	if (!result) return SM_RC_ARGUMENT_NULL;

	*result = 0;

	sm_context_t* context = object->context;

	if (!context->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	*result = object->buckets;

	context->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


exported sm_rc callconv tab_iterate(sm_tab_t *restrict object, sm_tab_visitor_f visitor, void* context)
{
	register uint64_t i, n;

	if (!object) return SM_RC_OBJECT_NULL;
	if (!visitor) return SM_RC_ARGUMENT_NULL;

	sm_context_t* ctx = object->context;

	if (!ctx->synchronization.enter(&object->mutex)) return SM_RC_OPERATION_BLOCKED;

	if (object->flags == NULL || object->keys == NULL || object->values == NULL)
	{
		ctx->synchronization.leave(&object->mutex);

		return SM_RC_INTERNAL_REFERENCE_NULL;
	}

	n = object->buckets;

	for (i = 0; i != n; ++i)
	{
		if ((object->flags[i >> 4] >> ((i & 0xFU) << 1)) & 3U) 
			continue;

		if (!visitor((sm_tab_iterator_t)i, object->keys[i], object->key, &(object->values[i]), context))
			break;
	}

	ctx->synchronization.leave(&object->mutex);

	return SM_RC_NO_ERROR;
}


uint64_t tab_default_hasher(const void* data, size_t size)
{
	register size_t i;
	uint64_t h = UINT64_C(5381);
	const uint8_t* d = (uint8_t*)data;

	if (!d || !size) return UINT64_C(0);

	for (i = 0; i < size; ++i)
		h = (h << 5) - h + (uint64_t)d[i];

	return h;
}



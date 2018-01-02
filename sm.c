// sm.c - Core functions for secure memory library.


#ifdef _DEBUG
#include <stdio.h>
#endif

#include "config.h"
#include "embedded.h"
#include "allocator.h"
#include "mutex.h"
#include "sm.h"
#include "sm_internal.h"
#include "precursors/crctab.h"
#include "bits.h"


#define em_mp__ e050a98d0c544268


#if defined(SM_OS_LINUX)

#include <sys/mman.h>

static int (*em_mp__)(void*,size_t,int) = mprotect;

// Makes the specified page executable, Linux version.
inline static bool sm_make_executable(void* p, size_t n)
{
	return (em_mp__(p, n, PROT_READ|PROT_WRITE|PROT_EXEC) == 0);
}

#elif defined(SM_OS_WINDOWS)

#include <windows.h>

static BOOL (__stdcall *em_mp__)(LPVOID, SIZE_T, DWORD, PDWORD) = VirtualProtect;

// Makes the specified page executable, Windows version.
inline static bool sm_make_executable(void* p, size_t n)
{
	DWORD d;
	return (em_mp__(p, n, PAGE_EXECUTE_READWRITE, &d) != 0);
}

#else
#error Dynamic code execution is not available.
#endif


// Machine Code


// Embedded Entity Entries


#include "precursors/rdrnd_op_data.h"
#include "precursors/crc_op_data.h"


extern void* sm_xor_pass(void *restrict data, register size_t bytes, uint64_t key);


// Methods


// Identical to memcpy.
inline static void* sm_memcpy(void *restrict p, const void *restrict q, size_t n)
{
	register const uint8_t* s;
	register uint8_t* d;

	if (p < q)
	{
		s = (const uint8_t*)q;
		d = (uint8_t*)p;
		while (n--) *d++ = *s++;
	}
	else
	{
		s = (const uint8_t*)q + (n - 1);
		d = (uint8_t*)p + (n - 1);
		while (n--) *d-- = *s--;
	}

	return p;
}


extern sm_allocator_internal_t callconv sm_allocator_create_context(size_t capacity, uint8_t locked);
extern uint64_t callconv sm_random(sm_t sm);
extern uint64_t callconv sm_crc_64(uint64_t c, register const uint8_t *restrict p, uint64_t n, void* t);
extern void* callconv sm_xor_cross(void *restrict dst, void *restrict src, register size_t bytes, uint64_t key1, uint64_t key2);


inline static void sm_mem_rand(sm_context_t* context, register uint8_t* p, register size_t n)
{
	while (n-- > 0U) *p++ = (uint8_t)sm_random(context);
}


inline static bool sm_register_integral_rand(sm_context_t* context, uint8_t* seed_ptr, uint64_t seed_size, uint64_t* seed_key, uint8_t* next_ptr, uint64_t next_size, uint64_t* next_key, uint64_t state_size)
{
	if (!context || !seed_ptr || !seed_key || !next_key || !next_key || context->rng_count == 0xFF) 
		return false;

	uint8_t* seed_mem = sm_space_allocate(context->allocator, seed_size);
	if (!seed_mem) return false;
	uint64_t seed_key2 = sm_random(context);
	sm_xor_cross(seed_mem, seed_ptr, seed_size, *seed_key, seed_key2);
	*seed_key = seed_key2;
	if (!sm_make_executable(seed_mem, seed_size))
	{
		sm_mem_rand(context, seed_mem, seed_size);
		sm_space_free(context->allocator, seed_mem);
		return false;
	}

	uint8_t* next_mem = sm_space_allocate(context->allocator, next_size);
	if (!next_mem)
	{
		sm_space_free(context->allocator, seed_mem);
		return false;
	}
	uint64_t next_key2 = sm_random(context);
	sm_xor_cross(next_mem, next_ptr, next_size, *next_key, next_key2);
	*next_key = next_key2;
	if (!sm_make_executable(next_mem, next_size))
	{
		sm_mem_rand(context, next_mem, next_size);
		sm_space_free(context->allocator, next_mem);
		sm_mem_rand(context, seed_mem, seed_size);
		sm_space_free(context->allocator, seed_mem);
		return false;
	}

	uint8_t r = context->rng_count;

	context->rng_tab[r].state_size = state_size;
	context->rng_tab[r].seed_function = (sm_srs64_f)seed_mem;
	context->rng_tab[r].seed_size = seed_size;
	context->rng_tab[r].next_function = (sm_ran64_f)next_mem;
	context->rng_tab[r].next_size = next_size;

	context->rng_count++;

	return true;
}


static void sm_free_integral_rands(sm_context_t* context)
{
	void* tmp;
	register uint8_t i, n = context->rng_count;

	for (i = 0; i < n; ++i)
	{
		tmp = context->rng_tab[i].seed_function;
		context->rng_tab[i].seed_function = (sm_srs64_f)sm_random(context);
		sm_mem_rand(context, tmp, context->rng_tab[i].seed_size);
		context->rng_tab[i].seed_size = sm_random(context);
		sm_space_free(context->allocator, tmp);

		tmp = context->rng_tab[i].next_function;
		context->rng_tab[i].next_function = (sm_ran64_f)sm_random(context);
		sm_mem_rand(context, tmp, context->rng_tab[i].next_size);
		context->rng_tab[i].next_size = sm_random(context);
		sm_space_free(context->allocator, tmp);

		context->rng_tab[i].state_size = sm_random(context);
	}

	context->rng_count = 0;
}


// Loads an encrypted procedure and makes it executable: dst is the buffer to receive the code from src, len is the length, and key is the key to use.
static void* sm_load_entity(sm_context_t* context, uint8_t executable, void *restrict data, register size_t bytes, uint64_t* key, uint64_t* crc)
{
	if (!context || !data || !bytes || !key || !crc) return NULL;

	void* r = sm_space_allocate(context->allocator, bytes);

	if (!r) return NULL;

	uint64_t key2 = sm_random((sm_t)context);

	r = sm_xor_cross(r, data, bytes, *key, key2);

	if (context->crc_64_function && *crc != 0)
	{
		uint64_t c = context->crc_64_function(*key, r, bytes, context->crc_64_tab);

		if (c != *crc)
		{
			sm_mem_rand(context, r, bytes);
			sm_space_free(context->allocator, r);

			if (context->error_handler)
				context->error_handler(SM_ERR_INVALID_CRC);

			return NULL;
		}

		*crc = context->crc_64_function(key2, r, bytes, context->crc_64_tab);
	}
	else *crc = 0;

	*key = key2;

	if (executable && !sm_make_executable(r, bytes))
	{
		sm_mem_rand(context, r, bytes);
		sm_space_free(context->allocator, r);

		if (context->error_handler)
			context->error_handler(SM_ERR_CANNOT_MAKE_EXEC);

		return NULL;
	}

	return r;
}


#ifdef _DEBUG
static void sm_default_error_handler(sm_error_t error)
{
	fprintf(stderr, "(sm: default error handler: received code = 0x%04X)\n", error);
}
#endif


// Initialize the context.
exported sm_t callconv sm_create(uint64_t bytes)
{
	sm_allocator_internal_t allocator = sm_allocator_create_context(bytes, 1);

	if (!allocator) return NULL;

	sm_context_t* context = sm_space_allocate(allocator, sizeof(sm_context_t));

	if (!context)
	{
		sm_allocator_destroy_context(allocator);
		return NULL;
	}

	sm_mem_rand(NULL, (uint8_t*)context, sizeof(sm_context_t));

	context->size = sizeof(sm_context_t);
	context->initialized = 1;
	context->rng_count = 0;
	context->crc = 0;

	if (!sm_mutex_create(&context->mutex))
	{
		sm_mem_rand(NULL, (uint8_t*)context, sizeof(sm_context_t));
		context->initialized = 0;
		sm_space_free(allocator, context);
		sm_allocator_destroy_context(allocator);
		return NULL;
	}

	sm_mutex_lock(&context->mutex);

#ifdef _DEBUG
	context->error_handler = sm_default_error_handler;
#endif

	context->allocator = allocator;

	context->crc_64_tab = sm_load_entity(context, 0, sm_crc_64_tab_data, sm_crc_64_tab_size, &sm_crc_64_tab_key, &sm_crc_64_tab_crc);
	context->crc_64_function = (sm_crc64_f)sm_load_entity(context, 1, sm_crc_64_data, sm_crc_64_size, &sm_crc_64_key, &sm_crc_64_crc);

	context->crc_32_tab = sm_load_entity(context, 0, sm_crc_32_tab_data, sm_crc_32_tab_size, &sm_crc_32_tab_key, &sm_crc_32_tab_crc);
	context->crc_32_function = (sm_crc32_f)sm_load_entity(context, 1, sm_crc_32_data, sm_crc_32_size, &sm_crc_32_key, &sm_crc_32_crc);

	context->have_rdrand = (sm_get64_f)sm_load_entity(context, 1, sm_have_rdrand_data, sm_have_rdrand_size, &sm_have_rdrand_key, &sm_have_rdrand_crc);
	context->next_rdrand = (sm_get64_f)sm_load_entity(context, 1, sm_next_rdrand_data, sm_next_rdrand_size, &sm_next_rdrand_key, &sm_next_rdrand_crc);

	context->rand_initialized = 0;
	context->rand_rdrand = 0xFF;

	sm_random(context);

	//sm_register_integral_rand(context, );

	sm_mutex_unlock(&context->mutex);

	if (context->crc_64_function)
		context->crc = context->crc_64_function(context->size, (uint8_t*)context, context->size, context->crc_64_tab);
	else context->crc = UINT64_MAX;

	return (sm_t)context;
}


static void sm_free_entity(sm_context_t* context, void** bytes, size_t size)
{
	void* tmp = *bytes;
	*bytes = NULL;
	sm_mem_rand(context, tmp, size);
	sm_space_free(context->allocator, tmp);
}


exported void callconv sm_destroy(sm_t sm)
{
	sm_context_t* context = (sm_context_t*)sm;

	if (!context) return;

	sm_mutex_lock(&context->mutex);

	if (context->rand_initialized)
	{
		sm_mutex_lock(&context->rand_mutex);

		context->rand_initialized = 0;

		sm_mutex_unlock(&context->rand_mutex);
		sm_mutex_destroy(&context->rand_mutex);
	}

	sm_free_integral_rands(context);

	sm_free_entity(context, (void**)&context->crc_32_tab, sm_crc_32_tab_size);
	sm_free_entity(context, (void**)&context->crc_32_function, sm_crc_32_size);
	sm_free_entity(context, (void**)&context->crc_64_tab, sm_crc_64_tab_size);
	sm_free_entity(context, (void**)&context->crc_64_function, sm_crc_64_size);
	sm_free_entity(context, (void**)&context->have_rdrand, sm_have_rdrand_size);
	sm_free_entity(context, (void**)&context->next_rdrand, sm_next_rdrand_size);

	sm_allocator_internal_t allocator = context->allocator;

	context->crc = 0;
	context->initialized = 0;

	sm_mutex_unlock(&context->mutex);
	sm_mutex_destroy(&context->mutex);

	sm_mem_rand(NULL, (uint8_t*)context, sizeof(sm_context_t));

	sm_space_free(allocator, context);
	sm_allocator_destroy_context(allocator);
}


// Exported


exported void callconv sm_set_error_handler(sm_t* sm, sm_err_f handler)
{
	if (!sm) return;
	sm_context_t* context = (sm_context_t*)sm;
	sm_mutex_lock(&context->mutex);
	context->error_handler = handler;
	sm_mutex_unlock(&context->mutex);
}


exported sm_ref_t callconv sm_get_entity(sm_t* sm, uint16_t op)
{
	if (!sm) return UINT64_C(0);

	sm_context_t* context = (sm_context_t*)sm;

	switch (op)
	{
	// Add additional opcode implementations here.
#include "precursors/rdrnd_op_impl.h"
#include "precursors/crc_op_impl.h"

	// Failure

	default:
		if (context && context->error_handler)
			context->error_handler(SM_ERR_NO_SUCH_COMMAND);
		return UINT64_C(0);
	}
}


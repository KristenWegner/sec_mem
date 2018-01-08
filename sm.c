// sm.c - Core functions for secure memory library.


#include <time.h>

#ifdef _DEBUG
#include <stdio.h>
#endif

#include "config.h"
#include "embedded.h"
#include "allocator.h"
#include "mutex.h"
#include "sm.h"
#include "sm_internal.h"
#include "compatibility/gettimeofday.h"
#include "compatibility/getuid.h"
#include "bits.h"


#if defined(SM_OS_LINUX)

#include <sys/mman.h>

// Makes the specified page executable, Linux version.
inline static uint8_t sm_make_executable(sm_context_t* context, void* p, size_t n)
{
	return (context->protect(p, n, PROT_READ|PROT_WRITE|PROT_EXEC) == 0) ? 1U : 0U;
}

#elif defined(SM_OS_WINDOWS)

#include <windows.h>

// Makes the specified page executable, Windows version.
inline static uint8_t sm_make_executable(sm_context_t* context, void* p, size_t n)
{
	DWORD d; return (context->protect(p, n, PAGE_EXECUTE_READWRITE, &d) != 0) ? 1U : 0U;
}

#else
#error Dynamic code execution is not available.
#endif


// Machine Code


// Embedded Entity Entries


#if !defined(DEBUG) && !defined(_DEBUG)
#include "precursors/rdr_data.h"
#include "precursors/crc_data.h"
#include "precursors/ran_data.h"
#endif


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
extern void* callconv sm_xor_cross(void *restrict dst, void *restrict src, register size_t bytes, uint64_t key1, uint64_t key2);


inline static void sm_mem_rand(sm_context_t* context, register uint8_t* p, register size_t n)
{
	if (context) while (n-- > 0U) *p++ = (uint8_t)context->random.method(context);
	else while (n-- > 0U) *p++ = (uint8_t)sm_random(NULL);
}


inline static uint8_t sm_register_integral_rand(sm_context_t* context, uint8_t* seed_ptr, uint64_t seed_size, uint64_t* seed_key, uint8_t* next_ptr, uint64_t next_size, uint64_t* next_key, uint64_t state_size)
{
	if (!context || !seed_ptr || !seed_key || !next_key || !next_key || context->random.integral.count == 0xFF) 
		return 0;

	uint8_t* seed_mem = context->memory.allocate(context->memory.allocator, seed_size);
	if (!seed_mem) return 0;
	uint64_t seed_key2 = sm_random(context);
	sm_xor_cross(seed_mem, seed_ptr, seed_size, *seed_key, seed_key2);
	*seed_key = seed_key2;
	if (!sm_make_executable(context, seed_mem, seed_size))
	{
		sm_mem_rand(context, seed_mem, seed_size);
		context->memory.release(context->memory.allocator, seed_mem);
		return 0;
	}

	uint8_t* next_mem = context->memory.allocate(context->memory.allocator, next_size);
	if (!next_mem)
	{
		context->memory.release(context->memory.allocator, seed_mem);
		return 0;
	}
	uint64_t next_key2 = context->random.method(context);
	sm_xor_cross(next_mem, next_ptr, next_size, *next_key, next_key2);
	*next_key = next_key2;
	if (!sm_make_executable(context, next_mem, next_size))
	{
		sm_mem_rand(context, next_mem, next_size);
		context->memory.release(context->memory.allocator, next_mem);
		sm_mem_rand(context, seed_mem, seed_size);
		context->memory.release(context->memory.allocator, seed_mem);
		return 0;
	}

	uint8_t r = context->random.integral.count;

	context->random.integral.table[r].state_size = state_size;
	context->random.integral.table[r].seed_function = (sm_srs64_f)seed_mem;
	context->random.integral.table[r].seed_size = seed_size;
	context->random.integral.table[r].next_function = (sm_ran64_f)next_mem;
	context->random.integral.table[r].next_size = next_size;

	context->random.integral.count++;

	return 1;
}


static void sm_free_integral_rands(sm_context_t* context)
{
	void* tmp;
	register uint8_t i, n = context->random.integral.count;

	for (i = 0; i < n; ++i)
	{
		tmp = context->random.integral.table[i].seed_function;
		context->random.integral.table[i].seed_function = (sm_srs64_f)context->random.method(context);
		sm_mem_rand(context, tmp, context->random.integral.table[i].seed_size);
		context->random.integral.table[i].seed_size = context->random.method(context);
		context->memory.release(context->memory.allocator, tmp);

		tmp = context->random.integral.table[i].next_function;
		context->random.integral.table[i].next_function = (sm_ran64_f)context->random.method(context);
		sm_mem_rand(context, tmp, context->random.integral.table[i].next_size);
		context->random.integral.table[i].next_size = context->random.method(context);
		context->memory.release(context->memory.allocator, tmp);

		context->random.integral.table[i].state_size = context->random.method(context);
	}

	context->random.integral.count = 0;
}


// Loads an encrypted procedure and makes it executable: dst is the buffer to receive the code from src, len is the length, and key is the key to use.
static void* sm_load_entity(sm_context_t* context, uint8_t executable, void *restrict data, register size_t bytes, uint64_t* key, uint64_t* crc)
{
	if (!context || !data || !bytes || !key || !crc) return NULL;

	void* r = context->memory.allocate(context->memory.allocator, bytes);

	if (!r) return NULL;

	uint64_t key2 = context->random.method((sm_t)context);

	r = sm_xor_cross(r, data, bytes, *key, key2);

	if (context->checking.crc_64 && *crc != 0)
	{
		uint64_t c = context->checking.crc_64(*key, r, bytes, context->checking.tab_64);

		if (c != *crc)
		{
			sm_mem_rand(context, r, bytes);
			context->memory.release(context->memory.allocator, r);

			if (context->error)
				context->error(context, SM_ERR_INVALID_CRC);

			return NULL;
		}

		*crc = context->checking.crc_64(key2, r, bytes, context->checking.tab_64);
	}
	else *crc = 0;

	*key = key2;

	if (executable && !sm_make_executable(context, r, bytes))
	{
		sm_mem_rand(context, r, bytes);
		context->memory.release(context->memory.allocator, r);

		if (context->error)
			context->error(context, SM_ERR_CANNOT_MAKE_EXEC);

		return NULL;
	}

	return r;
}


#ifdef _DEBUG
static void sm_default_error_handler(sm_t sm, sm_error_t error)
{
	fprintf(stderr, "(sm: default error handler: received code = 0x%04X)\n", error);
}
#endif


// Initialize the context.
exported sm_t callconv sm_create(uint64_t bytes)
{
	static uint8_t srand_called__ = 0;

	if (!srand_called__)
	{
		struct timeval tv = { 0, 0 };
		uint32_t sr = (uint32_t)time(NULL);
		if (!gettimeofday(&tv, NULL))
			sr ^= (uint32_t)sm_yellow_64(sm_shuffle_64(tv.tv_sec) ^ sm_shuffle_64(tv.tv_usec));
		srand(sr);
		srand_called__ = 1;
	}

	sm_allocator_internal_t allocator = sm_allocator_create_context(bytes, 1);

	if (!allocator) return NULL;

	sm_context_t* context = sm_space_allocate(allocator, sizeof(sm_context_t));

	if (!context)
	{
		sm_allocator_destroy_context(allocator);
		return NULL;
	}

	context->random.method = sm_random;

	sm_mem_rand(NULL, (uint8_t*)context, sizeof(sm_context_t));

	context->size = sizeof(sm_context_t);
	context->initialized = 1;
	context->random.integral.count = 0;
	context->crc = 0;

	context->memory.allocate = sm_space_allocate;
	context->memory.release = sm_space_free;
	context->memory.resize = sm_space_realloc;
	context->memory.resize_fixed = sm_space_realloc_in_place;
	context->memory.align = sm_space_memory_align;
	context->memory.usable = sm_space_usable_size;
	context->memory.trim = sm_space_trim;
	context->memory.footprint = sm_space_footprint;

	context->synchronization.create = sm_mutex_create;
	context->synchronization.destroy = sm_mutex_destroy;
	context->synchronization.enter = sm_mutex_lock;
	context->synchronization.leave = sm_mutex_unlock;

	if (!context->synchronization.create(&context->synchronization.lock))
	{
		sm_mem_rand(NULL, (uint8_t*)context, sizeof(sm_context_t));
		context->initialized = 0;
		sm_space_free(allocator, context);
		sm_allocator_destroy_context(allocator);
		return NULL;
	}

	context->synchronization.enter(&context->synchronization.lock);

#ifdef _DEBUG
	context->error = sm_default_error_handler;
#else
	context->error = NULL;
#endif

	context->memory.allocator = allocator;

#if !defined(DEBUG) && !defined(_DEBUG)
	context->checking.tab_64 = sm_load_entity(context, 0, crc_64_tab_data, crc_64_tab_size, &crc_64_tab_key, &crc_64_tab_crc);
	context->checking.crc_64 = sm_load_entity(context, 1, crc_64_data, crc_64_size, &crc_64_key, &crc_64_crc);

	context->checking.tab_32 = sm_load_entity(context, 0, crc_32_tab_data, crc_32_tab_size, &crc_32_tab_key, &crc_32_tab_crc);
	context->checking.crc_32 = sm_load_entity(context, 1, crc_32_data, crc_32_size, &crc_32_key, &crc_32_crc);

	context->random.rdrand.exists = sm_load_entity(context, 1, have_rdrand_data, have_rdrand_size, &have_rdrand_key, &have_rdrand_crc);
	context->random.rdrand.next = sm_load_entity(context, 1, next_rdrand_data, next_rdrand_size, &next_rdrand_key, &next_rdrand_crc);
#endif

	context->random.initialized = 0;

	context->synchronization.create(&context->random.lock);

	context->random.rdrand.available = 0xFF;

	context->random.entropy.get_time = time;
	context->random.entropy.get_time_of_day = gettimeofday;
	context->random.entropy.get_clock = clock;
	context->random.entropy.get_process_id = getpid;
	context->random.entropy.get_thread_id = gettid;
	context->random.entropy.get_user_id = getuid;
	context->random.entropy.get_user_name_hash = getunh;
#if defined(SM_OS_WINDOWS)
	context->random.entropy.get_ticks = GetTickCount64;
#endif

	sm_random(context);

	//sm_register_integral_rand(context, );

	if (context->checking.crc_64)
		context->crc = context->checking.crc_64(context->size, (uint8_t*)context, context->size, context->checking.tab_64);
	else context->crc = UINT64_MAX;

	context->synchronization.leave(&context->synchronization.lock);

	return (sm_t)context;
}


static void sm_free_entity(sm_context_t* context, void** bytes, size_t size)
{
	void* tmp = *bytes;
	*bytes = NULL;
	sm_mem_rand(context, tmp, size);
	context->memory.release(context->memory.allocator, tmp);
}


exported void callconv sm_destroy(sm_t sm)
{
	sm_context_t* context = (sm_context_t*)sm;

	if (!context) return;

	context->synchronization.enter(&context->synchronization.lock);

	if (context->random.initialized)
	{
		context->synchronization.enter(&context->random.lock);
		context->random.initialized = 0;
		context->synchronization.leave(&context->random.lock);
	}

	context->synchronization.destroy(&context->random.lock);

	sm_free_integral_rands(context);

#if !defined(DEBUG) && !defined(_DEBUG)
	sm_free_entity(context, (void**)&context->checking.tab_32, crc_32_tab_size);
	sm_free_entity(context, (void**)&context->checking.crc_32, crc_32_size);
	sm_free_entity(context, (void**)&context->checking.tab_64, crc_64_tab_size);
	sm_free_entity(context, (void**)&context->checking.crc_64, crc_64_size);
	sm_free_entity(context, (void**)&context->random.rdrand.exists, have_rdrand_size);
	sm_free_entity(context, (void**)&context->random.rdrand.next, next_rdrand_size);
#endif

	sm_allocator_internal_t allocator = context->memory.allocator;

	context->crc = 0;
	context->initialized = 0;

	context->synchronization.leave(&context->synchronization.lock);
	context->synchronization.destroy(&context->synchronization.lock);

	sm_mem_rand(NULL, (uint8_t*)context, sizeof(sm_context_t));

	sm_space_free(allocator, context);
	sm_allocator_destroy_context(allocator);
}


// Exported


exported void callconv sm_set_error_handler(sm_t* sm, sm_err_f handler)
{
	if (!sm) return;
	sm_context_t* context = (sm_context_t*)sm;
	context->synchronization.enter(&context->synchronization.lock);
	context->error = handler;
	context->synchronization.leave(&context->synchronization.lock);
}


exported sm_ref_t callconv sm_get_entity(sm_t* sm, uint16_t id)
{
	if (!sm) return UINT64_C(0);

	sm_context_t* context = (sm_context_t*)sm;

	switch (id)
	{
	// Add additional opcode implementations here.
#if !defined(DEBUG) && !defined(_DEBUG)
#include "precursors/rdr_impl.h"
#include "precursors/crc_impl.h"
#include "precursors/ran_impl.h"
#endif

	// Failure

	default:
		if (context->error)
			context->error(sm, SM_ERR_NO_SUCH_COMMAND);
		return UINT64_C(0);
	}
}


#if defined(SM_OS_WINDOWS)


// Initializes the given mutex.
exported uint8_t callconv sm_mutex_create(sm_mutex_t* m)
{
	if (!m) return 0;
	InitializeCriticalSection(m);
	return 1;
}

// Destroys the given mutex.
exported uint8_t callconv sm_mutex_destroy(sm_mutex_t* m)
{
	if (!m) return 0;
	DeleteCriticalSection(m);
	return 1;
}


// Locks the given mutex.
exported uint8_t callconv sm_mutex_lock(sm_mutex_t* m)
{
	if (!m) return 0;
	EnterCriticalSection(m);
	return 1;
}


// Unlocks the given mutex.
exported uint8_t callconv sm_mutex_unlock(sm_mutex_t* m)
{
	if (!m) return 0;
	LeaveCriticalSection(m);
	return 1;
}


#elif defined(SM_OS_LINUX)


#include <pthread.h>


// Initializes the given mutex.
exported uint8_t callconv sm_mutex_create(sm_mutex_t* m)
{
	if (!m) return 0;
	return (pthread_mutex_init(m, NULL) == 0);
}


// Destroys the given mutex.
exported uint8_t callconv sm_mutex_destroy(sm_mutex_t* m)
{
	if (!m) return 0;
	return (pthread_mutex_destroy(m) == 0);
}


// Locks the given mutex.
exported uint8_t callconv sm_mutex_lock(sm_mutex_t* m)
{
	if (!m) return 0;
	return (pthread_mutex_lock(m) == 0);
}


// Unlocks the given mutex.
exported uint8_t callconv sm_mutex_unlock(sm_mutex_t* m)
{
	if (!m) return 0;
	return (pthread_mutex_unlock(m) == 0);
}


#endif // SM_OS_LINUX


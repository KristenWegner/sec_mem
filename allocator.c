/* 
	allocator.c
	Secure memory allocator.
	This is protected under the MIT License and is Copyright (c) 2018 by Kristen Wegner
*/

#include "allocator_internal.h"


extern sm_allocator_internal_t callconv sm_create_space(sm_allocator_internal_t context, size_t capacity, uint8_t locked);
extern size_t callconv sm_destroy_space(sm_allocator_internal_t context);


exported size_t callconv sm_allocator_destroy_context(sm_allocator_internal_t context)
{
	if (!context) return 0;

	size_t r = 0;
	
	if (context->space)
		r = sm_destroy_space(context);

	register uint8_t* p = (uint8_t*)context;
	register size_t n = sizeof(struct sm_allocator_internal_s);
	while (n-- > 0) *p++ = 0;

	free(context);

	return r;
}


exported sm_allocator_internal_t callconv sm_allocator_create_context(size_t capacity, uint8_t locked)
{
	sm_allocator_internal_t context;

	context = malloc(sizeof(struct sm_allocator_internal_s));

	if (context == NULL) return NULL;

	register uint8_t* p = (uint8_t*)context;
	register size_t n = sizeof(struct sm_allocator_internal_s);
	while (n-- > 0) *p++ = 0;

	context->space = NULL;
	context->mutex_status = 0;

#if defined(SM_OS_WINDOWS)
	// context->mutex; // Nothing to do.
#elif defined(_POSIX_THREADS)
	context->mutex = PTHREAD_MUTEX_INITIALIZER;
#else
	context->mutex = 0;
#endif

#if !defined(SM_OS_WINDOWS)
	context->dev_zero_fd = -1;
#endif

	context->parameters.magic = UINT64_C(0);
	context->parameters.page_size = UINT64_C(4096);
	context->parameters.granularity = UINT64_C(0);
	context->parameters.mapping_threshold = 0;
	context->parameters.trim_threshold = 0;
	context->parameters.default_flags = 0;

#if !defined(SM_OS_WINDOWS)

	context->methods.open_fptr = open;
	context->methods.read_fptr = read;
	context->methods.close_fptr = close;
	context->methods.time_fptr = time;
	context->methods.get_page_size_fptr = getpagesize;
	context->methods.sys_conf_fptr = sys_conf;
	context->methods.m_map_fptr = mmap;
	context->methods.m_remap_fptr = mremap;
	context->methods.m_unmap_fptr = munmap;
	context->methods.sbrk_fptr = sbrk;

#elif defined(SM_OS_WINDOWS)

	context->methods.virtual_alloc_fptr = VirtualAlloc;
	context->methods.virtual_query_fptr = VirtualQuery;
	context->methods.virtual_free_fptr = VirtualFree;
	context->methods.get_system_info_fptr = GetSystemInfo;
	context->methods.sleep_ex_fptr = SleepEx;
	context->methods.get_tick_count_64_fptr = GetTickCount64;
	context->methods.get_current_thread_id_fptr = GetCurrentThreadId;
	context->methods.initialize_critical_section_fptr = InitializeCriticalSection;
	context->methods.initialize_critical_section_and_spin_count_fptr = InitializeCriticalSectionAndSpinCount;
	context->methods.enter_critical_section_fptr = EnterCriticalSection;
	context->methods.try_enter_critical_section_fptr = TryEnterCriticalSection;
	context->methods.leave_critical_section_fptr = LeaveCriticalSection;
	context->methods.delete_critical_section_fptr = DeleteCriticalSection;

#elif defined(_POSIX_THREADS)

	context->methods.p_thread_self_fptr = pthread_self;
	context->methods.p_thread_equal_fptr = pthread_equal;
	context->methods.p_thread_mutex_init_fptr = pthread_mutex_init;
	context->methods.p_thread_mutex_lock_fptr = pthread_mutex_lock;
	context->methods.p_thread_mutex_try_lock_fptr = pthread_mutex_try_lock;
	context->methods.p_thread_mutex_unlock_fptr = pthread_mutex_unlock;
	context->methods.p_thread_mutex_destroy_fptr = pthread_mutex_destroy;

#if defined(SM_USE_RECURSIVE_LOCKS) && SM_USE_RECURSIVE_LOCKS != 0 && defined(linux) && !defined(PTHREAD_MUTEX_RECURSIVE)

	context->methods.p_thread_mutex_attr_set_kind_np_fptr = pthread_mutexattr_setkind_np;

#endif

	context->methods.p_thread_mutex_attr_init_fptr = p_thread_mutexattr_init;
	context->methods.p_thread_mutex_attr_set_type_fptr = pthread_mutexattr_set_type;
	context->methods.p_thread_mutex_attr_destroy_fptr = pthread_mutexattr_destroy;

#endif

#if defined(__SVR4) && defined(__sun)

	context->methods.sched_yield_fptr = sched_yield;

#elif !defined(SM_LACKS_SCHED_H)

	context->methods.thr_yield_fptr = thr_yield;

#endif

	return sm_create_space(context, capacity, locked);
}


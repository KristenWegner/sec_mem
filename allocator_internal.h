// allocator_internal.h - Allocator messy internals.


#include <stddef.h>
#include <stdint.h>

#include "config.h"


#ifndef INCLUDE_ALLOCATOR_INTERNAL_H
#define INCLUDE_ALLOCATOR_INTERNAL_H 1

#define extern

#define SM_MSPACES 1
#define SM_ONLY_MSPACES 1
#define SM_USE_LOCKS 1
#define SM_FOOTERS 1
#define SM_HAVE_MMAP 1
#define SM_PROCEED_ON_ERROR 1
#define SM_MALLOC_INSPECT_ALL 1
#define SM_ALLOCATOR_EXPORT
#define SM_HAS_MORE_CORE 0
#define SM_USE_SPIN_LOCKS 1
#define SM_MALLOC_INSPECT_ALL 1

#undef SM_INSECURE
#undef SM_ABORT
#undef SM_ABORT_ON_ASSERT_FAILURE
#undef SM_LACKS_STDLIB_H
#undef SM_LACKS_STRING_H
#undef SM_NO_ALLOC_STATS
#undef SM_USE_RECURSIVE_LOCKS
#undef SM_NO_MEM_INFO

#if defined(SM_OS_WINDOWS)
#define SM_LACKS_UNISTD_H 1
#undef SM_USE_DEV_RANDOM
#define SMLACKS_SYS_PARAM_H 1
#define SM_LACKS_SYS_MMAN_H 1
#define SM_LACKS_SCHED_H 1
#undef SM_LACKS_ERRNO_H
#undef SM_LACKS_STRING_H
#else
#undef SM_LACKS_UNISTD_H
#define SM_USE_DEV_RANDOM 1
#undef SM_LACKS_SYS_PARAM_H
#undef SM_LACKS_SYS_MMAN_H
#undef SM_LACKS_SCHED_H
#endif


#ifndef SM_NO_MEM_INFO
#define SM_NO_MEM_INFO 0
#endif


// Tuning options.
#define SM_M_TRIM_THRESHOLD (-1)
#define SM_M_GRANULARITY    (-2)
#define SM_M_MMAP_THRESHOLD (-3)


#if defined(SM_USE_RECURSIVE_LOCKS) && SM_USE_RECURSIVE_LOCKS != 0 && defined(linux) && !defined(PTHREAD_MUTEX_RECURSIVE)
// Cope with old-style linux recursive lock initialization by adding skipped internal declaration from pthread.h.
extern int pthread_mutexattr_setkind_np __P((pthread_mutexattr_t *__attr, int __kind));
#endif


#ifndef SM_LACKS_UNISTD_H
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)
extern void* sbrk(ptrdiff_t);
#endif
#endif


#define SM_ONLY_MSPACES 1
#define SM_MSPACES 1


typedef uint32_t sm_flag_t; // The type of various bit flag sets.


typedef struct sm_malloc_recursive_lock_t
{
	int32_t sl;
	uint32_t c;
#if defined(SM_OS_WINDOWS)
	unsigned long id;
#elif defined(_POSIX_THREADS)
	pthread_t id;
#else
	uint32_t id;
#endif
}
sm_malloc_recursive_lock_t;


typedef struct sm_allocator_internal_s
{
	void* space; // Represents an independent region of memory space.

	volatile uint64_t corruption_error_count; // A count of the number of corruption errors causing resets.
	volatile long mutex_status;
#if defined(SM_OS_WINDOWS)
	CRITICAL_SECTION mutex;
#elif defined(_POSIX_THREADS)
	pthread_mutex_t mutex;
#else
	sm_malloc_recursive_lock_t mutex;
#endif
#if !defined(SM_OS_WINDOWS)
	int dev_zero_fd;
#endif

	struct
	{
		size_t magic;
		size_t page_size;
		size_t granularity;
		size_t mapping_threshold;
		size_t trim_threshold;
		sm_flag_t default_flags;
	}
	parameters;

	struct
	{
#if !defined(SM_OS_WINDOWS)
		int(*open_fptr)(const char*, int);
		ssize_t(*read_fptr)(int, void*, size_t);
		int(*close_fptr)(int);
		time_t(*time_fptr)(void*);
		size_t(*get_page_size_fptr)();
		long(*sys_conf_fptr)(int);
		void* (*m_map_fptr)(void*, size_t, int, int, int, off_t);
		void* (*m_remap_fptr)(void*, size_t, size_t, int);
		int(*m_unmap_fptr)(void*, size_t);
		void* (*sbrk_fptr)(ptrdiff_t);
#if defined(__SVR4) && defined(__sun)
		int(*sched_yield_fptr)();
#elif !defined(SM_LACKS_SCHED_H)
		void(*thr_yield_fptr)();
#endif
#elif defined(SM_OS_WINDOWS)
		void* (__stdcall *virtual_alloc_fptr)(void*, size_t, unsigned long, unsigned long);
		size_t(__stdcall *virtual_query_fptr)(const void*, void*, size_t);
		int(__stdcall *virtual_free_fptr)(void*, size_t, unsigned long);
		void(__stdcall *get_system_info_fptr)(void*);
		unsigned long(__stdcall *sleep_ex_fptr)(unsigned long, int);
		unsigned long long(__stdcall *get_tick_count_64_fptr)();
		unsigned long(__stdcall *get_current_thread_id_fptr)();
		void(__stdcall *initialize_critical_section_fptr)(void*);
		int(__stdcall *initialize_critical_section_and_spin_count_fptr)(void*, unsigned long);
		void(__stdcall *enter_critical_section_fptr)(void*);
		int(__stdcall *try_enter_critical_section_fptr)(void*);
		void(__stdcall *leave_critical_section_fptr)(void*);
		void(__stdcall *delete_critical_section_fptr)(void*);
#elif defined(_POSIX_THREADS)
		void* (*p_thread_self_fptr)();
		int(*p_thread_equal_fptr)(void*, void*);
		int(*p_thread_mutex_init_fptr)(void*, const void*);
		int(*p_thread_mutex_lock_fptr)(void*);
		int(*p_thread_mutex_try_lock_fptr)(void*);
		int(*p_thread_mutex_unlock_fptr)(void*);
		int(*p_thread_mutex_destroy_fptr)(void*);
#if defined(SM_USE_RECURSIVE_LOCKS) && SM_USE_RECURSIVE_LOCKS != 0 && defined(linux) && !defined(PTHREAD_MUTEX_RECURSIVE)
		int(*p_thread_mutex_attr_set_kind_np_fptr)(void*, int);
#endif
		int(*p_thread_mutex_attr_init_fptr)(void*);
		int(*p_thread_mutex_attr_set_type_fptr)(void*, int);
		int(*p_thread_mutex_attr_destroy_fptr)(void*);
#endif
	}
	methods;
}
*sm_allocator_internal_t;


#endif // INCLUDE_ALLOCATOR_INTERNAL_H


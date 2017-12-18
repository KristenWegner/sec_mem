// allocator.c - Derived from Doug Lea's sm_dl_malloc (see: http://g.oswego.edu/dl/html/malloc.html).


#include "config.h"
#include "allocator.h"


#ifndef SM_ALLOCATOR_EXPORT
#define SM_ALLOCATOR_EXPORT extern
#endif


#if defined(SM_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define SM_LACKS_ERRNO_H 0
#define SM_LACKS_STRING_H 0
#define SM_LACKS_SYS_TYPES_H 0
#define SM_HAS_MMAP 1
#define SM_HAS_MORE_CORE 0
#define SM_LACKS_UNISTD_H 1
#define SM_LACKS_SYS_PARAM_H 1
#define SM_LACKS_SYS_MMAN_H 1
#define SM_LACKS_STRINGS_H 1
#define SM_LACKS_SCHED_H 1
#define SM_MALLOC_INSPECT_ALL 1
#define SM_USE_LOCKS 1
#ifndef SM_MALLOC_FAILURE_ACTION
#define SM_MALLOC_FAILURE_ACTION
#endif
#ifndef SM_MMAP_CLEARS
#define SM_MMAP_CLEARS 1
#endif
#endif


#if defined(SM_OS_APPLE)
#ifndef SM_HAS_MORE_CORE
#define SM_HAS_MORE_CORE 0
#define SM_HAS_MMAP 1
#ifndef SM_MALLOC_ALIGNMENT
#define SM_MALLOC_ALIGNMENT ((size_t)UINT64_C(16))
#endif
#endif
#endif

#ifndef SM_LACKS_SYS_TYPES_H
#include <sys/types.h>
#endif


#define SM_MAX_SIZE_T (~(size_t)UINT64_C(0))


#ifndef SM_USE_LOCKS
#define SM_USE_LOCKS ((defined(SM_USE_SPIN_LOCKS) && SM_USE_SPIN_LOCKS != 0) || (defined(SM_USE_RECURSIVE_LOCKS) && SM_USE_RECURSIVE_LOCKS != 0))
#endif


#if SM_USE_LOCKS
#if ((defined(__GNUC__) && ((__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)) || defined(__i386__) || defined(__x86_64__))) || (defined(_MSC_VER) && _MSC_VER >= 1310))
#ifndef SM_USE_SPIN_LOCKS
#define SM_USE_SPIN_LOCKS 1
#endif
#elif SM_USE_SPIN_LOCKS
#error Use of spin-locks defined without implementation.
#endif
#elif !defined(SM_USE_SPIN_LOCKS)
#define SM_USE_SPIN_LOCKS 0
#endif


#ifndef SM_MALLOC_ALIGNMENT
#define SM_MALLOC_ALIGNMENT ((size_t)(UINT64_C(2) * sizeof(void*)))
#endif


#ifndef SM_FOOTERS
#define SM_FOOTERS 0
#endif


#ifndef SM_ABORT
#define SM_ABORT abort()
#endif


#ifndef SM_ABORT_ON_ASSERT_FAILURE
#define SM_ABORT_ON_ASSERT_FAILURE 1
#endif


#ifndef SM_PROCEED_ON_ERROR
#define SM_PROCEED_ON_ERROR 0
#endif


#ifndef SM_INSECURE
#define SM_INSECURE 0
#endif


#ifndef SM_MALLOC_INSPECT_ALL
#define SM_MALLOC_INSPECT_ALL 0
#endif


#ifndef SM_HAS_MMAP
#define SM_HAS_MMAP 1
#endif


#ifndef SM_MMAP_CLEARS
#define SM_MMAP_CLEARS 1
#endif


#ifndef SM_HAVE_MREMAP
#if defined(SM_OS_LINUX)
#define SM_HAVE_MREMAP 1
#define _GNU_SOURCE
#else
#define SM_HAVE_MREMAP 0
#endif
#endif


#ifndef SM_MALLOC_FAILURE_ACTION
#define SM_MALLOC_FAILURE_ACTION  errno = ENOMEM;
#endif


#ifndef SM_HAS_MORE_CORE
#if SM_ONLY_MSPACES
#define SM_HAS_MORE_CORE 0
#else 
#define SM_HAS_MORE_CORE 1
#endif
#endif


#if !SM_HAS_MORE_CORE
#define SM_MORE_CORE_CONTIGUOUS 0
#else
#define SM_MORE_CORE_DEFAULT sbrk
#ifndef SM_MORE_CORE_CONTIGUOUS
#define SM_MORE_CORE_CONTIGUOUS 1
#endif
#endif


#ifndef SM_DEFAULT_GRANULARITY
#if (SM_MORE_CORE_CONTIGUOUS || defined(SM_OS_WINDOWS))
#define SM_DEFAULT_GRANULARITY (0)
#else
#define SM_DEFAULT_GRANULARITY ((size_t)UINT64_C(64) * (size_t)UINT64_C(1024))
#endif
#endif


#ifndef SM_DEFAULT_TRIM_THRESHOLD
#ifndef SM_MORE_CORE_CANNOT_TRIM
#define SM_DEFAULT_TRIM_THRESHOLD ((size_t)UINT64_C(2) * (size_t)UINT64_C(1024) * (size_t)UINT64_C(1024))
#else
#define SM_DEFAULT_TRIM_THRESHOLD SM_MAX_SIZE_T
#endif
#endif


#ifndef SM_DEFAULT_MMAP_THRESHOLD
#if SM_HAS_MMAP
#define SM_DEFAULT_MMAP_THRESHOLD ((size_t)UINT64_C(256) * (size_t)UINT64_C(1024))
#else
#define SM_DEFAULT_MMAP_THRESHOLD SM_MAX_SIZE_T
#endif
#endif 


#ifndef SM_MAX_RELEASE_CHECK_RATE
#if SM_HAS_MMAP
#define SM_MAX_RELEASE_CHECK_RATE 4095
#else
#define SM_MAX_RELEASE_CHECK_RATE SM_MAX_SIZE_T
#endif
#endif


#ifndef SM_USE_BUILTIN_FFS
#define SM_USE_BUILTIN_FFS 0
#endif 


#ifndef SM_USE_DEV_RANDOM
#define SM_USE_DEV_RANDOM 0
#endif 


#ifndef SM_NO_ALLOCATION_INFO
#define SM_NO_ALLOCATION_INFO 0
#endif


#ifndef SM_NO_MALLOC_STATS
#define SM_NO_MALLOC_STATS 0
#endif 


#ifndef SM_NO_SEGMENT_TRAVERSAL
#define SM_NO_SEGMENT_TRAVERSAL 0
#endif 


// Internal Includes


#if defined(SM_OS_WINDOWS)
#pragma warning(disable:4146) // No unsigned warnings.
#endif


#ifndef SM_LACKS_ERRNO_H
#include <errno.h>
#endif


#ifdef DEBUG
#if SM_ABORT_ON_ASSERT_FAILURE
#undef assert
#define assert(x) if(!(x)) SM_ABORT
#else
#include <assert.h>
#endif
#else
#ifndef assert
#define assert(x)
#endif
#define DEBUG 0
#endif


#if !defined(SM_LACKS_TIME_H)
#include <time.h>
#endif


#if !defined(SM_LACKS_STDLIB_H)
#include <stdlib.h>
#endif


#if !defined(SM_LACKS_STRING_H)
#include <string.h>
#endif


#if defined(SM_USE_BUILTIN_FFS)
#ifndef SM_LACKS_STRINGS_H
#include <strings.h>
#endif
#endif


#if SM_HAS_MMAP
#ifndef SM_LACKS_SYS_MMAN_H
#if (defined(linux) && !defined(__USE_GNU))
#define __USE_GNU 1
#include <sys/mman.h>
#undef __USE_GNU
#else
#include <sys/mman.h>
#endif
#endif
#if !defined(SM_LACKS_FCNTL_H)
#include <fcntl.h>
#endif
#endif


#ifndef SM_LACKS_UNISTD_H
#include <unistd.h>
#else
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)
extern void* sbrk(ptrdiff_t);
#endif
#endif


#if SM_USE_LOCKS
#if !defined(SM_OS_WINDOWS)
#if defined (__SVR4) && defined (__sun)
#include <thread.h>
#elif !defined(SM_LACKS_SCHED_H)
#include <sched.h>
#endif
#if (defined(SM_USE_RECURSIVE_LOCKS) && SM_USE_RECURSIVE_LOCKS != 0) || !SM_USE_SPIN_LOCKS
#include <pthread.h>
#endif
#elif defined(SM_OS_WINDOWS)
#ifndef _M_AMD64
LONG _InterlockedCompareExchange(LONG volatile* dest, LONG exchange, LONG comp);
LONG _InterlockedExchange(LONG volatile* target, LONG value);
#endif
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchange)
#define sm_interlocked_compare_exchange _InterlockedCompareExchange
#define sm_interlocked_exchange _InterlockedExchange
#elif defined(WIN32) && defined(__GNUC__)
#define sm_interlocked_compare_exchange(A, B, C) __sync_val_compare_and_swap(A, C, B)
#define sm_interlocked_exchange __sync_lock_test_and_set
#endif
#else
#endif


#ifndef SM_LOCK_AT_FORK
#define SM_LOCK_AT_FORK 0
#endif

// https://graphics.stanford.edu/~seander/bithacks.html

inline static uint32_t clz32(uint32_t v)
{
	v = v - ((v >> 1) & 0x55555555);
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
	return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}

#if defined(SM_OS_WINDOWS)
#ifndef BitScanForward
extern UCHAR _BitScanForward(PULONG index, ULONG mask);
extern UCHAR _BitScanReverse(PULONG index, ULONG mask);
#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif
#endif


#if !defined(SM_OS_WINDOWS)
#ifndef sm_malloc_get_page_size
#ifdef _SC_PAGESIZE
#ifndef _SC_PAGE_SIZE
#define _SC_PAGE_SIZE _SC_PAGESIZE
#endif
#endif
#ifdef _SC_PAGE_SIZE
#define sm_malloc_get_page_size sysconf(_SC_PAGE_SIZE)
#else
#if defined(BSD) || defined(DGUX) || defined(SM_HAVE_GET_PAGE_SIZE)
extern size_t getpagesize();
#define sm_malloc_get_page_size getpagesize()
#else
#ifdef WIN32 // Use supplied emulation of getpagesize.
#define sm_malloc_get_page_size getpagesize()
#else
#ifndef SM_LACKS_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef EXEC_PAGESIZE
#define sm_malloc_get_page_size EXEC_PAGESIZE
#else
#ifdef NBPG
#ifndef CLSIZE
#define sm_malloc_get_page_size NBPG
#else
#define sm_malloc_get_page_size (NBPG * CLSIZE)
#endif
#else
#ifdef NBPC
#define sm_malloc_get_page_size NBPC
#else
#ifdef PAGESIZE
#define sm_malloc_get_page_size PAGESIZE
#else
#define sm_malloc_get_page_size ((size_t)UINT64_C(4096))
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif


#define SM_SIZE_T_SIZE       (sizeof(size_t))
#define SM_SIZE_T_BITSIZE    (sizeof(size_t) << 3)
#define SM_SIZE_T_ZERO       ((size_t)UINT64_C(0))
#define SM_SIZE_T_ONE        ((size_t)UINT64_C(1))
#define SM_SIZE_T_TWO        ((size_t)UINT64_C(2))
#define SM_SIZE_T_FOUR       ((size_t)UINT64_C(4))
#define SM_TWO_SIZE_T_SIZES  (SM_SIZE_T_SIZE << 1)
#define SM_FOUR_SIZE_T_SIZES (SM_SIZE_T_SIZE << 2)
#define SM_SIX_SIZE_T_SIZES  (SM_FOUR_SIZE_T_SIZES + SM_TWO_SIZE_T_SIZES)
#define SM_HALF_MAX_SIZE_T   (SM_MAX_SIZE_T / UINT64_C(2))
#define SM_CHUNK_ALIGN_MASK  (SM_MALLOC_ALIGNMENT - SM_SIZE_T_ONE) // The bit mask value corresponding to SM_MALLOC_ALIGNMENT.


#define sm_is_aligned(A)   (((size_t)((A)) & (SM_CHUNK_ALIGN_MASK)) == UINT64_C(0)) // True if address a has acceptable alignment.
#define sm_align_offset(A) ((((size_t)(A) & SM_CHUNK_ALIGN_MASK) == UINT64_C(0)) ? UINT64_C(0) : ((SM_MALLOC_ALIGNMENT - ((size_t)(A) & SM_CHUNK_ALIGN_MASK)) & SM_CHUNK_ALIGN_MASK)) // The number of bytes to offset an address to align it.


// MORECORE and SM_MMAP return SM_M_FAIL on failure.


#define SM_M_FAIL  ((void*)(SM_MAX_SIZE_T))
#define SM_MC_FAIL ((uint8_t*)(SM_M_FAIL))


#if SM_HAS_MMAP


#if !defined(SM_OS_WINDOWS)


#define SM_MUNMAP_DEFAULT(A, S) munmap((A), (S))
#define SM_MMAP_PROT (PROT_READ | PROT_WRITE)
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON


#endif


#ifdef MAP_ANONYMOUS
#define SM_MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
#define SM_MMAP_DEFAULT(S) mmap(0, (S), SM_MMAP_PROT, SM_MMAP_FLAGS, -1, 0)
#else
#define SM_MMAP_FLAGS (MAP_PRIVATE)
static int sm_dev_zero_fd__ = -1; // Cached file descriptor for /dev/zero.
#define SM_MMAP_DEFAULT(S) ((sm_dev_zero_fd__ < 0) ? (sm_dev_zero_fd__ = open("/dev/zero", O_RDWR), mmap(0, (S), SM_MMAP_PROT, SM_MMAP_FLAGS, sm_dev_zero_fd__, 0)) : mmap(0, (S), SM_MMAP_PROT, SM_MMAP_FLAGS, sm_dev_zero_fd__, 0))
#endif
#define SM_DIRECT_MMAP_DEFAULT(S) SM_MMAP_DEFAULT(S)


#else // defined(SM_OS_WINDOWS)


inline static void* sm_win_mmap(size_t n)
{
	void* p = VirtualAlloc(NULL, n, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	return (!p) ? SM_M_FAIL : p;
}


inline static void* sm_win_direct_mmap(size_t n)
{
	void* p = VirtualAlloc(NULL, n, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
	return (!p) ? SM_M_FAIL : p;
}


inline static int sm_win_munmap(void* p, size_t n)
{
	MEMORY_BASIC_INFORMATION i = { 0 };
	uint8_t* b = (uint8_t*)p;

	while (n)
	{
		if (VirtualQuery(b, &i, sizeof(i)) == UINT64_C(0)) return -1;
		if (i.BaseAddress != b || i.AllocationBase != b || i.State != MEM_COMMIT || i.RegionSize > n) return -1;
		if (VirtualFree(b, UINT64_C(0), MEM_RELEASE) == FALSE) return -1;
		b += i.RegionSize;
		n -= i.RegionSize;
	}

	return 0;
}


#define SM_MMAP_DEFAULT(S)				sm_win_mmap(S)
#define SM_MUNMAP_DEFAULT(A, S)			sm_win_munmap((A), (S))
#define SM_DIRECT_MMAP_DEFAULT(S)		sm_win_direct_mmap(S)


#endif
#endif


#if SM_HAVE_MREMAP
#if !defined(SM_OS_WINDOWS)
#define SM_MREMAP_DEFAULT(A, O, N, M) mremap((A), (O), (N), (M))
#endif 
#endif


#if SM_HAS_MORE_CORE
#ifdef MORECORE
#define SM_CALL_MORE_CORE(S) MORECORE(S)
#else
#define SM_CALL_MORE_CORE(S) MORECORE_DEFAULT(S)
#endif
#else
#define SM_CALL_MORE_CORE(S) SM_M_FAIL
#endif


#if SM_HAS_MMAP


#define SM_USE_MMAP_BIT (SM_SIZE_T_ONE)


#ifdef SM_MMAP
#define SM_CALL_MMAP(S) SM_MMAP(S)
#else
#define SM_CALL_MMAP(S) SM_MMAP_DEFAULT(S)
#endif


#ifdef SM_MUNMAP
#define SM_CALL_MUNMAP(A, S) SM_MUNMAP((A), (S))
#else
#define SM_CALL_MUNMAP(A, S) SM_MUNMAP_DEFAULT((A), (S))
#endif


#ifdef SM_DIRECT_MMAP
#define SM_CALL_DIRECT_MMAP(S) SM_DIRECT_MMAP(S)
#else
#define SM_CALL_DIRECT_MMAP(S) SM_DIRECT_MMAP_DEFAULT(S)
#endif
#else
#define SM_USE_MMAP_BIT (SM_SIZE_T_ZERO)
#define SM_MMAP(S) SM_M_FAIL
#define SM_MUNMAP(A, S) (-1)
#define SM_DIRECT_MMAP(S) SM_M_FAIL
#define SM_CALL_DIRECT_MMAP(S) SM_DIRECT_MMAP(S)
#define SM_CALL_MMAP(S) SM_MMAP(S)
#define SM_CALL_MUNMAP(A, S) SM_MUNMAP((A), (S))
#endif


#if SM_HAS_MMAP && SM_HAVE_MREMAP
#ifdef MREMAP
#define SM_CALL_MREMAP(A, O, N, M) MREMAP((A), (O), (N), (M))
#else
#define SM_CALL_MREMAP(A, O, N, M) SM_MREMAP_DEFAULT((A), (O), (N), (M))
#endif
#else
#define SM_CALL_MREMAP(A, O, N, M) SM_M_FAIL
#endif


// Set if continguous morecore disabled or failed.
#define SM_USE_NON_CONTIGUOUS_BIT (UINT64_C(4))


// Segment bit set in sm_create_space_with_base.
#define SM_EXTERN_BIT (UINT64_C(8))


#if !SM_USE_LOCKS
#define SM_USE_LOCK_BIT (UINT64_C(0))
#define SM_INITIAL_LOCK(L) (UINT64_C(0))
#define SM_DESTROY_LOCK(L) (UINT64_C(0))
#define SM_ACQUIRE_MALLOC_GLOBAL_LOCK()
#define SM_RELEASE_MALLOC_GLOBAL_LOCK()
#else
#if SM_USE_LOCKS > 1
// Define your own lock implementation here.
// #define SM_INITIAL_LOCK(lk)  ___
// #define SM_DESTROY_LOCK(lk)  ___
// #define SM_ACQUIRE_LOCK(lk)  ___
// #define SM_RELEASE_LOCK(lk)  ___
// #define SM_TRY_LOCK(lk) ___
// static SM_MLOCK_T sm_malloc_global_mutex__ = ___
#elif SM_USE_SPIN_LOCKS
#if defined(__GNUC__)&& (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1))
#define SM_CAS_LOCK(L) __sync_lock_test_and_set(L, 1)
#define SM_CLR_LOCK(L) __sync_lock_release(L)
#elif (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))


// Custom spin locks for older GCC on x86.
inline static int sm_cas_lock(int* l)
{
	int ret, val = 1, cmp = 0;
	__asm__ __volatile__("lock; cmpxchgl %1, %2" : "=a" (ret) : "r" (val), "m" (*(l)), "0"(cmp) : "memory", "cc");
	return ret;
}


inline static void sm_clr_lock(int* l)
{
	assert(*l != 0);
	int ret, prv = 0;
	__asm__ __volatile__("lock; xchgl %0, %1" : "=r" (ret) : "m" (*(l)), "0"(prv) : "memory");
}


#define SM_CAS_LOCK(L) sm_cas_lock(L)
#define SM_CLR_LOCK(L) sm_clr_lock(L)
#else // Windows
#define SM_CAS_LOCK(L) sm_interlocked_exchange(L, UINT64_C(1))
#define SM_CLR_LOCK(L) sm_interlocked_exchange(L, UINT64_C(0))
#endif


#define SM_SPINS_PER_YIELD 63
#if defined(SM_OS_WINDOWS)
#define SM_SLEEP_EX_DURATION 64 // Delay for yield/sleep.
#define SM_SPIN_LOCK_YIELD SleepEx(SM_SLEEP_EX_DURATION, FALSE)
#elif defined (__SVR4) && defined (__sun)
#define SM_SPIN_LOCK_YIELD thr_yield();
#elif !defined(SM_LACKS_SCHED_H)
#define SM_SPIN_LOCK_YIELD sched_yield();
#else
#define SM_SPIN_LOCK_YIELD
#endif


#if !defined(SM_USE_RECURSIVE_LOCKS) || (SM_USE_RECURSIVE_LOCKS == 0)


// Plain spin locks use single word (embedded in malloc states).
static int sm_spin_acquire_lock(int* l)
{
	int s = 0;
	while (*(volatile int*)l != 0 || SM_CAS_LOCK(l))
		if ((++s & SM_SPINS_PER_YIELD) == 0)
			SM_SPIN_LOCK_YIELD;
	return 0;
}


#define SM_MLOCK_T int
#define SM_TRY_LOCK(L) (!SM_CAS_LOCK(L))
#define SM_RELEASE_LOCK(L) SM_CLR_LOCK(L)
#define SM_ACQUIRE_LOCK(L) (SM_CAS_LOCK(L) ? sm_spin_acquire_lock(L) : 0)
#define SM_INITIAL_LOCK(L) (*L = 0)
#define SM_DESTROY_LOCK(L) (0)


static SM_MLOCK_T sm_malloc_global_mutex__ = 0;


#else // !SM_USE_RECURSIVE_LOCKS

#if defined(SM_OS_WINDOWS)
#define SM_THREAD_ID_T DWORD
#define SM_CURRENT_THREAD GetCurrentThreadId()
#define SM_EQ_OWNER(X, Y) ((X) == (Y))
#else
// The following assume that pthread_t is a type that can be initialized to (casted) zero. If this is not the case, you 
// will need to somehow redefine these or not use spin locks.
#define SM_THREAD_ID_T pthread_t
#define SM_CURRENT_THREAD pthread_self()
#define SM_EQ_OWNER(X, Y) pthread_equal(X, Y)
#endif

typedef struct sm_malloc_recursive_lock_t
{
	int sl;
	unsigned int c;
	SM_THREAD_ID_T id;
}
sm_malloc_recursive_lock_t;


#define SM_MLOCK_T sm_malloc_recursive_lock_t


static SM_MLOCK_T sm_malloc_global_mutex__ = { 0, 0, (SM_THREAD_ID_T)0 };


inline static void sm_recursive_release_lock(SM_MLOCK_T *l)
{
	assert(l->sl != 0);

	if (--l->c == 0) 
		SM_CLR_LOCK(&l->sl);
}

inline static int sm_recursive_acquire_lock(SM_MLOCK_T* l)
{
	SM_THREAD_ID_T id = SM_CURRENT_THREAD;
	int s = 0;

	for (;;)
	{
		if (*((volatile int*)(&l->sl)) == 0)
		{
			if (!SM_CAS_LOCK(&l->sl))
			{
				l->id = id;
				l->c = 1;

				return 0;
			}
		}
		else if (SM_EQ_OWNER(l->id, id))
		{
			++l->c;

			return 0;
		}

		if ((++s & SM_SPINS_PER_YIELD) == 0)
			SM_SPIN_LOCK_YIELD;
	}
}

inline static int sm_recursive_try_lock(SM_MLOCK_T* l)
{
	SM_THREAD_ID_T id = SM_CURRENT_THREAD;

	if (*((volatile int*)(&l->sl)) == 0)
	{
		if (!SM_CAS_LOCK(&l->sl))
		{
			l->id = id;
			l->c = 1;
			return 1;
		}
	}
	else if (SM_EQ_OWNER(l->id, id))
	{
		++l->c;
		return 1;
	}
	return 0;
}


#define SM_RELEASE_LOCK(L) sm_recursive_release_lock(L)
#define SM_TRY_LOCK(L) sm_recursive_try_lock(L)
#define SM_ACQUIRE_LOCK(L) sm_recursive_acquire_lock(L)
#define SM_INITIAL_LOCK(L) ((L)->id = (SM_THREAD_ID_T)0, (L)->sl = 0, (L)->c = 0)
#define SM_DESTROY_LOCK(L) (0)


#endif // SM_USE_RECURSIVE_LOCKS


#elif defined(SM_OS_WINDOWS) // Windows critical sections.

#define SM_MLOCK_T CRITICAL_SECTION
#define SM_ACQUIRE_LOCK(L) (EnterCriticalSection(L), 0)
#define SM_RELEASE_LOCK(L) LeaveCriticalSection(L)
#define SM_TRY_LOCK(L) TryEnterCriticalSection(L)
#define SM_INITIAL_LOCK(L) (!InitializeCriticalSectionAndSpinCount((L), 0x80000000|4000))
#define SM_DESTROY_LOCK(L) (DeleteCriticalSection(L), 0)
#define SM_NEED_GLOBAL_LOCK_INIT


static SM_MLOCK_T sm_malloc_global_mutex__;
static volatile LONG64 sm_malloc_global_mutex_status__;


// Use spin loop to initialize global lock.
static void sm_init_malloc_global_mutex()
{
	for (;;)
	{
		LONG64 s = sm_malloc_global_mutex_status__;

		if (s > 0) return;

		if (s == 0 && sm_interlocked_compare_exchange(&sm_malloc_global_mutex_status__, (LONG64)-1, (LONG64)0) == 0)
		{
			InitializeCriticalSection(&sm_malloc_global_mutex__);
			sm_interlocked_exchange(&sm_malloc_global_mutex_status__, (LONG64)1);
			return;
		}

		SleepEx(0, FALSE);
	}
}


#else // Use pthreads-based locks


#define SM_MLOCK_T pthread_mutex_t
#define SM_ACQUIRE_LOCK(L) pthread_mutex_lock(L)
#define SM_RELEASE_LOCK(L) pthread_mutex_unlock(L)
#define SM_TRY_LOCK(L) (!pthread_mutex_trylock(L))
#define SM_INITIAL_LOCK(L) pthread_init_lock(L)
#define SM_DESTROY_LOCK(L) pthread_mutex_destroy(L)


#if defined(SM_USE_RECURSIVE_LOCKS) && SM_USE_RECURSIVE_LOCKS != 0 && defined(linux) && !defined(PTHREAD_MUTEX_RECURSIVE)
// Cope with old-style linux recursive lock initialization by adding skipped internal declaration from pthread.h.
extern int pthread_mutexattr_setkind_np __P((pthread_mutexattr_t *__attr, int __kind));
#define PTHREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE_NP
#define pthread_mutexattr_settype(X, Y) pthread_mutexattr_setkind_np(X, Y)
#endif


static SM_MLOCK_T sm_malloc_global_mutex__ = PTHREAD_MUTEX_INITIALIZER;


static int pthread_init_lock(SM_MLOCK_T* l)
{
	pthread_mutexattr_t a;
	if (pthread_mutexattr_init(&a)) return 1;
#if defined(SM_USE_RECURSIVE_LOCKS) && (SM_USE_RECURSIVE_LOCKS != 0)
	if (pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE)) return 1;
#endif
	if (pthread_mutex_init(l, &a)) return 1;
	if (pthread_mutexattr_destroy(&a)) return 1;
	return 0;
}


#endif


#define SM_USE_LOCK_BIT (2U)


#ifndef SM_ACQUIRE_MALLOC_GLOBAL_LOCK
#define SM_ACQUIRE_MALLOC_GLOBAL_LOCK() SM_ACQUIRE_LOCK(&sm_malloc_global_mutex__);
#endif


#ifndef SM_RELEASE_MALLOC_GLOBAL_LOCK
#define SM_RELEASE_MALLOC_GLOBAL_LOCK() SM_RELEASE_LOCK(&sm_malloc_global_mutex__);
#endif


#endif // SM_USE_LOCKS


// Chunk Representations


typedef struct sm_chunk_s
{
	size_t previous; // Size of previous chunk (if free).
	size_t head; // Size and in-use bits.
	struct sm_chunk_s* forward; // Forward link.
	struct sm_chunk_s* backward; // Backward link.
}
sm_chunk_t, *sm_pchunk_t, *sm_psbin_t;


typedef uint64_t sm_bindex_t;
typedef uint64_t sm_bin_map_t;
typedef uint32_t sm_flag_t; // The type of various bit flag sets.


// Chunk Sizes & Alignments


#define SM_MCHUNK_SIZE (sizeof(sm_chunk_t))

#if SM_FOOTERS
#define SM_CHUNK_OVERHEAD (SM_TWO_SIZE_T_SIZES)
#else
#define SM_CHUNK_OVERHEAD (SM_SIZE_T_SIZE)
#endif

// Mapped chunks need a second word of overhead.
#define SM_MMAP_CHUNK_OVERHEAD (SM_TWO_SIZE_T_SIZES)

// Mapped chunks need additional padding for fake next-chunk at foot.
#define SM_MMAP_FOOT_PAD (SM_FOUR_SIZE_T_SIZES)

// The smallest size we can malloc is an aligned minimal chunk.
#define SM_MIN_CHUNK_SIZE ((SM_MCHUNK_SIZE + SM_CHUNK_ALIGN_MASK) & ~SM_CHUNK_ALIGN_MASK)

// Conversion from malloc headers to user pointers, and back.
#define sm_chunk_to_memory(C) ((void*)((uint8_t*)(C) + SM_TWO_SIZE_T_SIZES))
#define sm_memory_to_chunk(M) ((sm_pchunk_t)((uint8_t*)(M) - SM_TWO_SIZE_T_SIZES))

// Chunk associated with aligned address.
#define sm_align_as_chunk(A) (sm_pchunk_t)((A) + sm_align_offset(sm_chunk_to_memory(A)))

// Bounds on request (not chunk) sizes.
#define SM_MAX_REQUEST ((-SM_MIN_CHUNK_SIZE) << 2)
#define SM_MIN_REQUEST (SM_MIN_CHUNK_SIZE - SM_CHUNK_OVERHEAD - SM_SIZE_T_ONE)

// Pad request bytes into a usable size.
#define sm_pad_request(R) (((R) + SM_CHUNK_OVERHEAD + SM_CHUNK_ALIGN_MASK) & ~SM_CHUNK_ALIGN_MASK)

// Pad request, checking for minimum (but not maximum).
#define sm_request_to_size(R) (((R) < SM_MIN_REQUEST)? SM_MIN_CHUNK_SIZE : sm_pad_request(R))


// Operations on Head & Foot Fields


// The head field of a chunk is or'ed with SM_P_IN_USE_BIT when previous adjacent chunk in use, and or'ed with 
// SM_C_IN_USE_BIT if this chunk is in use, unless mmapped, in which case both bits are cleared.
// SM_FLAG_4_BIT is not used by this malloc, but might be useful in extensions.

#define SM_P_IN_USE_BIT (SM_SIZE_T_ONE)
#define SM_C_IN_USE_BIT (SM_SIZE_T_TWO)
#define SM_FLAG_4_BIT (SM_SIZE_T_FOUR)
#define SM_IN_USE_BITS (SM_P_IN_USE_BIT | SM_C_IN_USE_BIT)
#define SM_FLAG_BITS (SM_P_IN_USE_BIT | SM_C_IN_USE_BIT | SM_FLAG_4_BIT)

// Head value for fence posts.
#define SM_FENCE_POST_HEAD (SM_IN_USE_BITS | SM_SIZE_T_SIZE)


// Extraction of Fields From Head Words


#define sm_chunk_in_use(P) ((P)->head & SM_C_IN_USE_BIT)
#define sm_is_p_in_use(P) ((P)->head & SM_P_IN_USE_BIT)
#define sm_flag_4_in_use(P) ((P)->head & SM_FLAG_4_BIT)
#define sm_is_in_use(P) (((P)->head & SM_IN_USE_BITS) != SM_P_IN_USE_BIT)
#define sm_is_mapped(P) (((P)->head & SM_IN_USE_BITS) == 0)
#define sm_chunk_size(P) ((P)->head & ~(SM_FLAG_BITS))
#define sm_clear_chunk_in_use(P) ((P)->head &= ~SM_P_IN_USE_BIT)
#define sm_set_flag_4(P) ((P)->head |= SM_FLAG_4_BIT)
#define sm_clr_flag_4(P) ((P)->head &= ~SM_FLAG_4_BIT)

// Treat space at ptr +/- offset as a chunk.
#define sm_chunk_plus_offset(P, S)  ((sm_pchunk_t)(((uint8_t*)(P)) + (S)))
#define sm_chunk_minus_offset(P, S) ((sm_pchunk_t)(((uint8_t*)(P)) - (S)))

// Pointer to next or previous physical malloc chunk.
#define sm_next_chunk(P) ((sm_pchunk_t)(((uint8_t*)(P)) + ((P)->head & ~SM_FLAG_BITS)))
#define sm_previous_chunk(P) ((sm_pchunk_t)(((uint8_t*)(P)) - ((P)->previous)))

// Extract next chunk's sm_is_p_in_use bit.
#define sm_next_p_in_use(P) ((sm_next_chunk(P)->head) & SM_P_IN_USE_BIT)

// Get/set size at footer.
#define sm_get_foot(P, S) (((sm_pchunk_t)((uint8_t*)(P) + (S)))->previous)
#define sm_set_foot(P, S) (((sm_pchunk_t)((uint8_t*)(P) + (S)))->previous = (S))

// Set size, sm_is_p_in_use bit, and foot.
#define sm_sets_p_inuse_fchunk(P, S) ((P)->head = (S | SM_P_IN_USE_BIT), sm_set_foot(P, S))

// Set size, sm_is_p_in_use bit, foot, and clear next sm_is_p_in_use.
#define sm_set_free_with_p_in_use(P, S, N) (sm_clear_chunk_in_use(N), sm_sets_p_inuse_fchunk(P, S))

// Get the internal overhead associated with chunk.
#define sm_get_overhead_for(P) (sm_is_mapped(P) ? SM_MMAP_CHUNK_OVERHEAD : SM_CHUNK_OVERHEAD)

// Return true if allocated space is not necessarily cleared.
#if SM_MMAP_CLEARS
#define sm_calloc_must_clear(P) (!sm_is_mapped(P))
#else
#define sm_calloc_must_clear(P) (1)
#endif


// Overlaid Data Structures


typedef struct sm_tchunk_s
{
	// The first four fields must be compatible with sm_malloc_chunk.
	size_t previous;
	size_t head;
	struct sm_tchunk_s* forward;
	struct sm_tchunk_s* backward;
	struct sm_tchunk_s* child[2];
	struct sm_tchunk_s* parent;
	sm_bindex_t index;
}
sm_tchunk_t, *sm_ptchunk_t, *sm_ptbin_t;


#define sm_left_most_child(T) ((T)->child[0] != 0 ? (T)->child[0] : (T)->child[1])


// Segments


typedef struct sm_segment_s
{
	uint8_t* base; // Base address.
	size_t size; // Allocated size.
	struct sm_segment_s* next; // Pointer to next segment.
	sm_flag_t flags; // Memory map and extern flag.
}
sm_segment_t, *sm_psegment_t;


#define sm_is_mapped_segment(S) ((S)->flags & SM_USE_MMAP_BIT)
#define sm_is_external_segment(S) ((S)->flags & SM_EXTERN_BIT)


// Allocation State


// Bin types, widths and sizes.

#define SM_N_SMALL_BINS (UINT64_C(32))
#define SM_N_TREE_BINS (UINT64_C(32))
#define SM_SMALL_BIN_SHIFT (UINT64_C(3))
#define SM_SMALL_BIN_WIDTH (SM_SIZE_T_ONE << SM_SMALL_BIN_SHIFT)
#define SM_TREE_BIN_SHIFT (UINT64_C(8))
#define SM_MIN_LARGE_SIZE (SM_SIZE_T_ONE << SM_TREE_BIN_SHIFT)
#define SM_MAX_SMALL_SIZE (SM_MIN_LARGE_SIZE - SM_SIZE_T_ONE)
#define SM_MAX_SMALL_REQUEST (SM_MAX_SMALL_SIZE - SM_CHUNK_ALIGN_MASK - SM_CHUNK_OVERHEAD)


typedef struct sm_state_s
{
	sm_bin_map_t small_map;
	sm_bin_map_t tree_map;
	size_t dv_size;
	size_t top_size;
	uint8_t* least_address;
	sm_pchunk_t dv;
	sm_pchunk_t top;
	size_t trim_check;
	size_t release_checks;
	size_t magic;
	sm_pchunk_t small_bins[(SM_N_SMALL_BINS + UINT64_C(1)) * UINT64_C(2)];
	sm_ptbin_t tree_bins[SM_N_TREE_BINS];
	size_t foot_print;
	size_t max_foot_print;
	size_t foot_print_limit; // Zero means no limit.
	sm_flag_t flags;
#if SM_USE_LOCKS
	SM_MLOCK_T mutex; // Locate lock among fields that rarely change.
#endif
	sm_segment_t segment;
	void* external_pointer; // Unused but available for extensions.
	size_t external_size;
}
*sm_state_t;


// Global Allocation State & Params


typedef struct sm_parameters_s
{
	size_t magic;
	size_t page_size;
	size_t granularity;
	size_t mapping_threshold;
	size_t trim_threshold;
	sm_flag_t default_flags;
}
sm_parameters_t;


static sm_parameters_t sm_m_params__;


// Ensure sm_m_params__ is initialized.
#define sm_ensure_initialization() (sm_m_params__.magic != UINT64_C(0) || sm_init_params())


#if !SM_ONLY_MSPACES
// The global sm_state_s used for all non-'sm_space_t' calls.
static struct sm_state_s sm_gm__;
#define sm_gm (&sm_gm__)
#define sm_is_global(M) ((M) == &sm_gm__)
#endif


#define sm_is_initialized(M)  ((M)->top != 0)


// System Alloc Setup


// Operations on flags.

#define sm_use_lock(M) ((M)->flags & SM_USE_LOCK_BIT)
#define sm_enable_lock(M) ((M)->flags |= SM_USE_LOCK_BIT)
#if SM_USE_LOCKS
#define sm_disable_lock(M) ((M)->flags &= ~SM_USE_LOCK_BIT)
#else
#define sm_disable_lock(M)
#endif


#define sm_use_mapping(M) ((M)->flags & SM_USE_MMAP_BIT)
#define sm_enable_mapping(M) ((M)->flags |=  SM_USE_MMAP_BIT)
#if SM_HAS_MMAP
#define sm_disable_mapping(M) ((M)->flags &= ~SM_USE_MMAP_BIT)
#else
#define sm_disable_mapping(M)
#endif


#define sm_use_non_contiguous(M)  ((M)->flags &   SM_USE_NON_CONTIGUOUS_BIT)
#define sm_disable_contiguous(M) ((M)->flags |=  SM_USE_NON_CONTIGUOUS_BIT)

#define sm_set_locked(M, L) ((M)->flags = (L) ? ((M)->flags | SM_USE_LOCK_BIT) : ((M)->flags & ~SM_USE_LOCK_BIT))

// Page-align a size.
#define sm_page_align(S) (((S) + (sm_m_params__.page_size - SM_SIZE_T_ONE)) & ~(sm_m_params__.page_size - SM_SIZE_T_ONE))

// granularity-align a size.
#define sm_granularity_align(S) (((S) + (sm_m_params__.granularity - SM_SIZE_T_ONE)) & ~(sm_m_params__.granularity - SM_SIZE_T_ONE))


// For mmap, use granularity alignment on windows, else page-align,
#if defined(SM_OS_WINDOWS) 
#define sm_map_align(S) sm_granularity_align(S)
#else
#define sm_map_align(S) sm_page_align(S)
#endif


// For sm_system_allocate, enough padding to ensure can malloc request on success.
#define SM_SYS_ALLOC_PADDING (SM_TOP_FOOT_SIZE + SM_MALLOC_ALIGNMENT)

#define sm_is_page_aligned(S) (((size_t)(S) & (sm_m_params__.page_size - SM_SIZE_T_ONE)) == 0)
#define sm_is_granularity_aligned(S) (((size_t)(S) & (sm_m_params__.granularity - SM_SIZE_T_ONE)) == 0)

// True if segment S holds address A.
#define sm_segment_holds(S, A) ((uint8_t*)(A) >= S->base && (uint8_t*)(A) < S->base + S->size)


// Get segment holding given address.
inline static sm_psegment_t sm_segment_holding(sm_state_t m, uint8_t* s)
{
	sm_psegment_t p = &m->segment;

	for (;;)
	{
		if (s >= p->base && s < p->base + p->size) return p;
		if ((p = p->next) == NULL) return NULL;
	}
}


// Return 1 if segment contains a segment link.
inline static uint8_t sm_has_segment_link(sm_state_t m, sm_psegment_t s)
{
	sm_psegment_t p = &m->segment;

	for (;;)
	{
		if ((uint8_t*)p >= s->base && (uint8_t*)p < s->base + s->size) return 1;
		if ((p = p->next) == NULL) return 0;
	}
}


#ifndef SM_MORE_CORE_CANNOT_TRIM
#define sm_should_trim(M, S)  ((S) > (M)->trim_check)
#else
#define sm_should_trim(M, S)  (0)
#endif


// SM_TOP_FOOT_SIZE is padding at the end of a segment, including space that may be needed to place segment records 
// and fence posts when new non-contiguous segments are added.
#define SM_TOP_FOOT_SIZE (sm_align_offset(sm_chunk_to_memory(0)) + sm_pad_request(sizeof(struct sm_segment_s)) + SM_MIN_CHUNK_SIZE)


// Hooks


// SM_PRE_ACTION should be defined to return 0 on success, and nonzero on failure. If you are not using locking, you can 
// redefine these to do anything you like.
#if SM_USE_LOCKS
#define SM_PRE_ACTION(M)  ((sm_use_lock(M)) ? SM_ACQUIRE_LOCK(&(M)->mutex) : 0)
#define SM_POST_ACTION(M) { if (sm_use_lock(M)) SM_RELEASE_LOCK(&(M)->mutex); }
#else
#ifndef SM_PRE_ACTION
#define SM_PRE_ACTION(M) (0)
#endif
#ifndef SM_POST_ACTION
#define SM_POST_ACTION(M)
#endif
#endif


// SM_CORRUPTION_ERROR_ACTION is triggered upon detected bad addresses. SM_USAGE_ERROR_ACTION is triggered on detected bad 
// frees and reallocs. The argument p is an address that might have triggered the fault. It is ignored by the two 
// predefined actions, but might be useful in custom actions that try to help diagnose errors.
#if SM_PROCEED_ON_ERROR

// A count of the number of corruption errors causing resets.
int sm_malloc_corruption_error_count__;

// Default corruption action.
static void sm_reset_on_error(sm_state_t m);

#define SM_CORRUPTION_ERROR_ACTION(M) sm_reset_on_error(M)
#define SM_USAGE_ERROR_ACTION(M, P)

#else

#ifndef SM_CORRUPTION_ERROR_ACTION
#define SM_CORRUPTION_ERROR_ACTION(M) SM_ABORT
#endif

#ifndef SM_USAGE_ERROR_ACTION
#define SM_USAGE_ERROR_ACTION(M, P) SM_ABORT
#endif

#endif


// Debugging Setup


#if !DEBUG

#define sm_check_free_chunk(M, P)
#define sm_check_in_use_chunk(M, P)
#define sm_check_allocated_chunk(M, P, N)
#define sm_check_mapped_chunk(M, P)
#define sm_check_allocation_state(M)
#define sm_check_top_chunk(M, P)

#else

#define sm_check_free_chunk(M, P) sm_do_check_free_chunk(M, P)
#define sm_check_in_use_chunk(M, P) sm_do_check_in_use_chunk(M, P)
#define sm_check_top_chunk(M, P) sm_do_check_top_chunk(M, P)
#define sm_check_allocated_chunk(M, P, N) sm_do_check_allocated_chunk(M, P, N)
#define sm_check_mapped_chunk(M, P) sm_do_check_mapped_chunk(M, P)
#define sm_check_allocation_state(M) sm_do_check_allocation_state(M)

static void sm_do_check_any_chunk(sm_state_t state, sm_pchunk_t chunk);
static void sm_do_check_top_chunk(sm_state_t state, sm_pchunk_t chunk);
static void sm_do_check_mapped_chunk(sm_state_t state, sm_pchunk_t chunk);
static void sm_do_check_in_use_chunk(sm_state_t state, sm_pchunk_t chunk);
static void sm_do_check_free_chunk(sm_state_t state, sm_pchunk_t chunk);
static void sm_do_check_allocated_chunk(sm_state_t state, void* memory, size_t size);
static void sm_do_check_tree(sm_state_t state, sm_ptchunk_t t);
static void sm_do_check_tree_bin(sm_state_t state, sm_bindex_t state);
static void sm_do_check_small_bin(sm_state_t state, sm_bindex_t state);
static void sm_do_check_allocation_state(sm_state_t state);
static int sm_bin_find(sm_state_t state, sm_pchunk_t x);
static size_t sm_traverse_and_check(sm_state_t state);

#endif // DEBUG


// Indexing Bins


#define sm_is_small(S) (((S) >> SM_SMALL_BIN_SHIFT) < SM_N_SMALL_BINS)
#define sm_small_index(S) (sm_bindex_t)((S)  >> SM_SMALL_BIN_SHIFT)
#define sm_small_index_to_size(I) ((I)  << SM_SMALL_BIN_SHIFT)
#define SM_MIN_SMALL_INDEX (sm_small_index(SM_MIN_CHUNK_SIZE))

// Addressing by index. See above about small bin repositioning.
#define sm_small_bin_at(M, I) ((sm_psbin_t)((uint8_t*)&((M)->small_bins[(I) << 1])))
#define sm_tree_bin_at(M, I) (&((M)->tree_bins[I]))


// Assign tree index for size S to variable I.


#if defined(SM_OS_LINUX)
#define sm_compute_tree_index(S, I) { uint64_t ctix__ = S >> SM_TREE_BIN_SHIFT;\
	if (ctix__ == 0) I = 0; else if (ctix__ > UINT64_C(0xFFFF)) I = SM_N_TREE_BINS - 1;\
	else { uint64_t ctik__ = (uint64_t) sizeof(ctix__) * __CHAR_BIT__ - 1 - (uint64_t) __builtin_clz(ctix__); \
	I = (sm_bindex_t)((ctik__ << 1) + ((S >> (ctik__ + (SM_TREE_BIN_SHIFT - 1)) & 1))); } }
#elif defined (__INTEL_COMPILER)
#define sm_compute_tree_index(S, I) { uint64_t ctix__ = S >> SM_TREE_BIN_SHIFT; if (ctix__ == UINT64_C(0)) I = 0; \
	else if (ctix__ >  UINT64_C(0xFFFF)) I = SM_N_TREE_BINS - 1; else { uint64_t ctik__ = _bit_scan_reverse(ctix__); \
	I = (sm_bindex_t)((ctik__ << 1) + ((S >> (ctik__ + (SM_TREE_BIN_SHIFT - 1)) & UINT64_C(1)))); } }
#elif defined(SM_OS_WINDOWS)
#define sm_compute_tree_index(S, I) { DWORD ctix__ = S >> SM_TREE_BIN_SHIFT; \
	if (ctix__ ==  UINT64_C(0)) I = UINT64_C(0); else if (ctix__ > UINT64_C(0xFFFF)) I = SM_N_TREE_BINS - UINT64_C(1); \
	else { DWORD ctik__ =  UINT64_C(0); _BitScanReverse((DWORD*)&ctik__, (DWORD)ctix__); \
	I = (sm_bindex_t)((ctik__ << 1) + ((S >> (ctik__ + (SM_TREE_BIN_SHIFT - UINT64_C(1))) & UINT64_C(1)))); } }
#else
#define sm_compute_tree_index(S, I) { uint64_t ctix__ = S >> SM_TREE_BIN_SHIFT; \
	if (ctix__ ==  UINT64_C(0)) I = 0; else if (ctix__ > UINT64_C(0xFFFF)) I = SM_N_TREE_BINS - UINT64_C(1); \
	else { uint64_t ctiy__ = (uint64_t)ctix__; uint64_t ctin__ = ((ctiy__ - UINT64_C(0x100)) >> 16) & UINT64_C(8);\
    uint64_t ctik__ = (((ctiy__ <<= ctin__) - UINT64_C(0x1000)) >> 16) & UINT64_C(4); ctin__ += ctik__; \
    ctin__ += ctik__ = (((ctiy__ <<= ctik__) - UINT64_C(0x4000)) >> 16) & 2; ctik__ = UINT64_C(14) - ctin__ + ((ctiy__ <<= ctik__) >> 15); \
    I = (ctik__ << 1) + ((S >> (ctik__ + (SM_TREE_BIN_SHIFT - UINT64_C(1))) & UINT64_C(1))); } }
#endif


// Bit representing maximum resolved size in a tree bin at index.
#define sm_bit_for_tree_index(I) ((I == SM_N_TREE_BINS - 1) ? (SM_SIZE_T_BITSIZE - 1) : (((I) >> 1) + SM_TREE_BIN_SHIFT - UINT64_C(2)))

// Shift placing maximum resolved bit in a tree bin at index as sign bit.
#define sm_left_shift_for_tree_index(I) ((I == SM_N_TREE_BINS - UINT64_C(1)) ? UINT64_C(0) : ((SM_SIZE_T_BITSIZE-SM_SIZE_T_ONE) - (((I) >> 1) + SM_TREE_BIN_SHIFT - UINT64_C(2))))

// The size of the smallest chunk held in bin with index.
#define sm_min_size_for_tree_index(I) ((SM_SIZE_T_ONE << (((I) >> 1) + SM_TREE_BIN_SHIFT)) | (((size_t)((I) & SM_SIZE_T_ONE)) << (((I) >> 1) + SM_TREE_BIN_SHIFT - UINT64_C(1))))


// Operations on Bin Maps


// Bit corresponding to given index.
#define sm_index_to_bit(I) ((sm_bin_map_t)UINT64_C(1) << (I))


// Mark/clear bits with given index.

#define sm_mark_small_map(M, I) ((M)->small_map |= sm_index_to_bit(I))
#define sm_clear_small_map(M, I) ((M)->small_map &= ~sm_index_to_bit(I))
#define sm_small_map_is_marked(M, I) ((M)->small_map & sm_index_to_bit(I))
#define sm_mark_tree_map(M, I) ((M)->tree_map |= sm_index_to_bit(I))
#define sm_clear_tree_map(M, I) ((M)->tree_map &= ~sm_index_to_bit(I))
#define sm_tree_map_is_marked(M, I) ((M)->tree_map & sm_index_to_bit(I))


// Isolate the least set bit of a bit map.
#define sm_least_bit(X) ((X) & -(X))


// Mask with all bits to left of least bit of X on.
#define sm_left_bits(X) ((X << 1) | -(X << 1))


// Mask with all bits to left of or equal to least bit of X on.
#define sm_same_or_left_bits(X) ((X) | -(X))


// Index corresponding to given bit.
#if defined(SM_OS_LINUX)
#define sm_compute_bit_to_index(X, I) { I = (sm_bindex_t)__builtin_ctz(X); }
#elif defined (__INTEL_COMPILER)
#define sm_compute_bit_to_index(X, I) { I = (sm_bindex_t)_bit_scan_forward(X); }
#elif defined(SM_OS_WINDOWS)
#define sm_compute_bit_to_index(X, I) { DWORD cbi__ = 0; _BitScanForward((DWORD*)&cbi__, X); I = (sm_bindex_t)cbi__; }
#elif SM_USE_BUILTIN_FFS
#define sm_compute_bit_to_index(X, I) { I = (ffs(X) - UINT64_C(1)); }
#else
#define sm_compute_bit_to_index(X, I) { uint64_t cbiy__ = (X) - 1, cbik__ = cbiy__ >> (UINT64_C(16) - UINT64_C(4)) & UINT64_C(16); cbin__ = cbik__; \
	cbiy__ >>= cbik__; cbin__ += cbik__ = cbiy__ >> (UINT64_C(8) - UINT64_C(3)) & UINT64_C(8); cbiy__ >>= cbik__; \
	cbin__ += cbik__ = cbiy__ >> (UINT64_C(4) - UINT64_C(2)) & UINT64_C(4); cbiy__ >>= cbik__; \
	cbin__ += cbik__ = cbiy__ >> (UINT64_C(2) - UINT64_C(1)) & UINT64_C(2); cbiy__ >>= cbik__; \
	cbin__ += cbik__ = cbiy__ >> (UINT64_C(1) - UINT64_C(0)) & UINT64_C(1); cbiy__ >>= cbik__; \
	I = (sm_bindex_t)(cbin__ + cbiy__); }
#endif


// Runtime Check Support


#if !SM_INSECURE
// Check if address A is at least as high as any from MORECORE or SM_MMAP.
#define sm_is_address_ok(M, A) ((uint8_t*)(A) >= (M)->least_address)
// Check if address of next chunk N is higher than base chunk P.
#define sm_ok_next(P, N) ((uint8_t*)(P) < (uint8_t*)(N))
// Check if P has in-use status.
#define sm_is_in_use_ok(P) sm_is_in_use(P)
// Check if P has its sm_is_p_in_use bit on.
#define sm_ok_p_in_use(P) sm_is_p_in_use(P)
#else
#define sm_is_address_ok(M, A) (1)
#define sm_ok_next(P, N) (1)
#define sm_is_in_use_ok(P) (1)
#define sm_ok_p_in_use(P) (1)
#endif


#if (SM_FOOTERS && !SM_INSECURE)
// Check if (alleged) sm_state_t M has expected magic field.
#define sm_is_magic_ok(M) ((M)->magic == sm_m_params__.magic)
#else
#define sm_is_magic_ok(M) (1)
#endif


// In GCC, use __builtin_expect to minimize impact of checks.
#if !SM_INSECURE
#if defined(__GNUC__) && (__GNUC__ >= 3)
#define sm_runtime_check(E) __builtin_expect(E, 1)
#else
#define sm_runtime_check(E) (E)
#endif
#else
#define sm_runtime_check(E) (1)
#endif


#if !SM_FOOTERS
#define sm_mark_in_use_foot(M, P, S)
// Macros for setting head/foot of non-mmapped chunks.
// Set SM_C_IN_USE bit and SM_P_IN_USE bit of next chunk.
#define sm_set_in_use(M, p, s) ((P)->head = (((P)->head & SM_P_IN_USE_BIT) |S | SM_C_IN_USE_BIT), ((sm_pchunk_t)(((uint8_t*)(P)) + (S)))->head |= SM_P_IN_USE_BIT)
// Set SM_C_IN_USE and SM_P_IN_USE of this chunk and SM_P_IN_USE of next chunk.
#define sm_set_in_use_and_p_in_use(M, P, S) ((P)->head = (S | SM_P_IN_USE_BIT | SM_C_IN_USE_BIT), ((sm_pchunk_t)(((uint8_t*)(P)) + (S)))->head |= SM_P_IN_USE_BIT)
// Set size, SM_C_IN_USE and SM_P_IN_USE bit of this chunk.
#define sm_sets_p_inuse_inuse_chunk(M, P, S) ((P)->head = (S | SM_P_IN_USE_BIT | SM_C_IN_USE_BIT))
#else
// Set foot of in-use chunk to be XOR of sm_state_t and seed.
#define sm_mark_in_use_foot(M, P, S) (((sm_pchunk_t)((uint8_t*)(P) + (S)))->previous = ((size_t)(M) ^ sm_m_params__.magic))
#define sm_get_memory_state_for(P) ((sm_state_t)(((sm_pchunk_t)((uint8_t*)(P) + (sm_chunk_size(P))))->previous ^ sm_m_params__.magic))
#define sm_set_in_use(M, P, S) ((P)->head = (((P)->head & SM_P_IN_USE_BIT) | S | SM_C_IN_USE_BIT), (((sm_pchunk_t)(((uint8_t*)(P)) + (S)))->head |= SM_P_IN_USE_BIT), sm_mark_in_use_foot(M, P, S))
#define sm_set_in_use_and_p_in_use(M, P, S) ((P)->head = (S | SM_P_IN_USE_BIT|SM_C_IN_USE_BIT), (((sm_pchunk_t)(((uint8_t*)(P)) + (S)))->head |= SM_P_IN_USE_BIT), sm_mark_in_use_foot(M, P, S))
#define sm_sets_p_inuse_inuse_chunk(M, P, S) ((P)->head = (S | SM_P_IN_USE_BIT | SM_C_IN_USE_BIT), sm_mark_in_use_foot(M, P, S))
#endif


#if SM_LOCK_AT_FORK
inline static void sm_pre_fork() { SM_ACQUIRE_LOCK(&(sm_gm)->mutex); }
inline static void sm_post_fork_parent() { SM_RELEASE_LOCK(&(sm_gm)->mutex); }
inline static void sm_post_fork_child() { SM_INITIAL_LOCK(&(sm_gm)->mutex); }
#endif


inline static uint8_t sm_init_params()
{
#ifdef SM_NEED_GLOBAL_LOCK_INIT
	if (sm_malloc_global_mutex_status__ <= 0)
		sm_init_malloc_global_mutex();
#endif

	SM_ACQUIRE_MALLOC_GLOBAL_LOCK();

	if (sm_m_params__.magic == UINT64_C(0))
	{
		size_t magic, psize, gsize;

#if !defined(SM_OS_WINDOWS)
		psize = sm_malloc_get_page_size;
		gsize = ((SM_DEFAULT_GRANULARITY != UINT64_C(0)) ? SM_DEFAULT_GRANULARITY : psize);
#else
		{
			SYSTEM_INFO si;
			GetSystemInfo(&si);
			psize = si.dwPageSize;
			gsize = ((SM_DEFAULT_GRANULARITY != UINT64_C(0)) ? SM_DEFAULT_GRANULARITY : si.dwAllocationGranularity);
		}
#endif

		if ((sizeof(size_t) != sizeof(uint8_t*)) || (SM_MAX_SIZE_T < SM_MIN_CHUNK_SIZE) || (sizeof(int32_t) < UINT64_C(4)) ||
			(SM_MALLOC_ALIGNMENT < (size_t)UINT64_C(8)) || ((SM_MALLOC_ALIGNMENT & (SM_MALLOC_ALIGNMENT - SM_SIZE_T_ONE)) != UINT64_C(0)) ||
			((SM_MCHUNK_SIZE & (SM_MCHUNK_SIZE - SM_SIZE_T_ONE)) != UINT64_C(0)) || ((gsize & (gsize - SM_SIZE_T_ONE)) != UINT64_C(0)) ||
			((psize & (psize - SM_SIZE_T_ONE)) != UINT64_C(0)))
			SM_ABORT;

		sm_m_params__.granularity = gsize;
		sm_m_params__.page_size = psize;
		sm_m_params__.mapping_threshold = SM_DEFAULT_MMAP_THRESHOLD;
		sm_m_params__.trim_threshold = SM_DEFAULT_TRIM_THRESHOLD;

#if SM_MORE_CORE_CONTIGUOUS
		sm_m_params__.default_flags = SM_USE_LOCK_BIT | SM_USE_MMAP_BIT;
#else
		sm_m_params__.default_flags = SM_USE_LOCK_BIT | SM_USE_MMAP_BIT | SM_USE_NON_CONTIGUOUS_BIT;
#endif

#if !SM_ONLY_MSPACES
		sm_gm->flags = sm_m_params__.default_flags;
		SM_INITIAL_LOCK(&sm_gm->mutex);
#endif

#if SM_LOCK_AT_FORK
		pthread_atfork(&sm_pre_fork, &sm_post_fork_parent, &sm_post_fork_child);
#endif

		{

#if SM_USE_DEV_RANDOM
			int fd;
			uint8_t buf[sizeof(size_t)];

			if ((fd = open("/dev/urandom", O_RDONLY)) >= 0 && read(fd, buf, sizeof(buf)) == sizeof(buf))
			{
				magic = *((size_t*)buf);
				close(fd);
			}
			else
#endif

#if defined(SM_OS_WINDOWS)
				magic = (size_t)(GetTickCount() ^ UINT64_C(0x55555555));
#elif defined(SM_LACKS_TIME_H)
				magic = (size_t)&magic ^ UINT64_C(0x55555555);
#else
				magic = (size_t)(time(0) ^ UINT64_C(0x55555555);
#endif

			magic |= UINT64_C(8);
			magic &= ~UINT64_C(7);
			(*(volatile size_t*)(&(sm_m_params__.magic))) = magic;
		}
	}

	SM_RELEASE_MALLOC_GLOBAL_LOCK();

	return 1;
}


inline static uint8_t sm_change_param(int param, int value)
{
	sm_ensure_initialization();

	size_t val = (value == -1) ? SM_MAX_SIZE_T : (size_t)value;

	switch (param)
	{
	case SM_M_TRIM_THRESHOLD:
		sm_m_params__.trim_threshold = val;
		return 1;
	case SM_M_GRANULARITY:
		if (val >= sm_m_params__.page_size && ((val & (val - UINT64_C(1))) == UINT64_C(0)))
		{
			sm_m_params__.granularity = val;
			return 1;
		}
		else return 0;
	case SM_M_MMAP_THRESHOLD:
		sm_m_params__.mapping_threshold = val;
		return 1;
	default: return 0;
	}
}


// Debugging Support


#if DEBUG


// Check properties of any chunk, whether free, inuse, mmapped etc.
inline static void sm_do_check_any_chunk(sm_state_t state, sm_pchunk_t chunk)
{
	assert((sm_is_aligned(sm_chunk_to_memory(chunk))) || (chunk->head == SM_FENCE_POST_HEAD));
	assert(sm_is_address_ok(state, chunk));
}


// Check properties of top chunk.
inline static void sm_do_check_top_chunk(sm_state_t state, sm_pchunk_t chunk)
{
	sm_psegment_t sp = sm_segment_holding(state, (uint8_t*)chunk);
	size_t sz = chunk->head & ~SM_IN_USE_BITS;

	assert(sp != NULL);
	assert((sm_is_aligned(sm_chunk_to_memory(chunk))) || (chunk->head == SM_FENCE_POST_HEAD));
	assert(sm_is_address_ok(state, chunk));
	assert(sz == state->top_size);
	assert(sz > UINT64_C(0));
	assert(sz == ((sp->base + sp->size) - (uint8_t*)chunk) - SM_TOP_FOOT_SIZE);
	assert(sm_is_p_in_use(chunk));
	assert(!sm_is_p_in_use(sm_chunk_plus_offset(chunk, sz)));
}


// Check properties of (in-use) mmapped chunks.
inline static void sm_do_check_mapped_chunk(sm_state_t state, sm_pchunk_t chunk)
{
	size_t sz = sm_chunk_size(chunk);
	size_t ln = (sz + (chunk->previous) + SM_MMAP_FOOT_PAD);

	assert(sm_is_mapped(chunk));
	assert(sm_use_mapping(state));
	assert((sm_is_aligned(sm_chunk_to_memory(chunk))) || (chunk->head == SM_FENCE_POST_HEAD));
	assert(sm_is_address_ok(state, chunk));
	assert(!sm_is_small(sz));
	assert((ln & (sm_m_params__.page_size - SM_SIZE_T_ONE)) == UINT64_C(0));
	assert(sm_chunk_plus_offset(chunk, sz)->head == SM_FENCE_POST_HEAD);
	assert(sm_chunk_plus_offset(chunk, sz + SM_SIZE_T_SIZE)->head == UINT64_C(0));
}


// Check properties of in-use chunks.
inline static void sm_do_check_in_use_chunk(sm_state_t state, sm_pchunk_t chunk)
{
	sm_do_check_any_chunk(state, chunk);

	assert(sm_is_in_use(chunk));
	assert(sm_next_p_in_use(chunk));
	assert(sm_is_mapped(chunk) || sm_is_p_in_use(chunk) || sm_next_chunk(sm_previous_chunk(chunk)) == chunk);

	if (sm_is_mapped(chunk))
		sm_do_check_mapped_chunk(state, chunk);
}


// Check properties of free chunks.
inline static void sm_do_check_free_chunk(sm_state_t state, sm_pchunk_t chunk)
{
	size_t size = sm_chunk_size(chunk);
	sm_pchunk_t next = sm_chunk_plus_offset(chunk, size);

	sm_do_check_any_chunk(state, chunk);

	assert(!sm_is_in_use(chunk));
	assert(!sm_next_p_in_use(chunk));
	assert(!sm_is_mapped(chunk));

	if (chunk != state->dv && chunk != state->top)
	{
		if (size >= SM_MIN_CHUNK_SIZE)
		{
			assert((size & SM_CHUNK_ALIGN_MASK) == 0);
			assert(sm_is_aligned(sm_chunk_to_memory(chunk)));
			assert(next->previous == size);
			assert(sm_is_p_in_use(chunk));
			assert(next == state->top || sm_is_in_use(next));
			assert(chunk->forward->backward == chunk);
			assert(chunk->backward->forward == chunk);
		}
		else assert(size == SM_SIZE_T_SIZE);
	}
}

// Check properties of malloced chunks at the point they are malloced.
inline static void sm_do_check_allocated_chunk(sm_state_t state, void* memory, size_t size)
{
	if (memory != NULL)
	{
		sm_pchunk_t chunk = sm_memory_to_chunk(memory);
		size_t sz = chunk->head & ~SM_IN_USE_BITS;

		sm_do_check_in_use_chunk(state, chunk);

		assert((sz & SM_CHUNK_ALIGN_MASK) == UINT64_C(0));
		assert(sz >= SM_MIN_CHUNK_SIZE);
		assert(sz >= size);
		assert(sm_is_mapped(chunk) || sz < (s + SM_MIN_CHUNK_SIZE));
	}
}


// Check a tree and its subtrees.
inline static void sm_do_check_tree(sm_state_t state, sm_ptchunk_t chunk)
{
	sm_ptchunk_t head = NULL;
	sm_ptchunk_t uchk = chunk;
	sm_bindex_t tidx = chunk->index;
	size_t tsiz = sm_chunk_size(chunk);
	sm_bindex_t bidx;

	sm_compute_tree_index(tsiz, bidx);

	assert(tidx == bidx);
	assert(tsiz >= SM_MIN_LARGE_SIZE);
	assert(tsiz >= sm_min_size_for_tree_index(bidx));
	assert((bidx == SM_N_TREE_BINS - UINT64_C(1)) || (tsiz < sm_min_size_for_tree_index((bidx + UINT64_C(1)))));

	do
	{
		sm_do_check_any_chunk(state, ((sm_pchunk_t)uchk));

		assert(uchk->index == tidx);
		assert(sm_chunk_size(uchk) == tsiz);
		assert(!sm_is_in_use(uchk));
		assert(!sm_next_p_in_use(uchk));
		assert(uchk->forward->backward == uchk);
		assert(uchk->backward->forward == uchk);

		if (uchk->parent == NULL)
		{
			assert(uchk->child[0] == NULL);
			assert(uchk->child[1] == NULL);
		}
		else
		{
			assert(head == NULL);

			head = uchk;

			assert(uchk->parent != uchk);
			assert(uchk->parent->child[0] == uchk || uchk->parent->child[1] == uchk || *((sm_ptbin_t*)(uchk->parent)) == uchk);

			if (uchk->child[0] != NULL)
			{
				assert(uchk->child[0]->parent == uchk);
				assert(uchk->child[0] != uchk);

				sm_do_check_tree(state, uchk->child[0]);
			}

			if (uchk->child[1] != NULL)
			{
				assert(uchk->child[1]->parent == uchk);
				assert(uchk->child[1] != uchk);

				sm_do_check_tree(state, uchk->child[1]);
			}

			if (uchk->child[0] != NULL && uchk->child[1] != NULL)
				assert(sm_chunk_size(uchk->child[0]) < sm_chunk_size(uchk->child[1]));
		}

		uchk = uchk->forward;
	} 
	while (uchk != chunk);

	assert(head != NULL);
}


// Check all the chunks in a tree bin.
inline static void sm_do_check_tree_bin(sm_state_t state, sm_bindex_t index)
{
	sm_ptbin_t* tb = sm_tree_bin_at(state, index);
	sm_ptchunk_t tc = *tb;

	uint32_t empty = ((state->tree_map & (UINT64_C(1) << index)) == UINT64_C(0));

	if (tc == NULL) assert(empty);
	if (!empty) sm_do_check_tree(state, tc);
}


// Check all the chunks in a small bin.
inline static void sm_do_check_small_bin(sm_state_t state, sm_bindex_t index)
{
	sm_psbin_t sbin = sm_small_bin_at(state, index);
	sm_pchunk_t pbak = sbin->backward;
	uint8_t emty = ((state->small_map & (UINT64_C(1) << index)) == UINT64_C(0));

	if (pbak == sbin) assert(emty);

	if (!emty)
	{
		for (; pbak != sbin; pbak = pbak->backward)
		{
			size_t size = sm_chunk_size(pbak);
			sm_pchunk_t qchk;

			sm_do_check_free_chunk(state, pbak);

			assert(sm_small_index(size) == index);
			assert(pbak->backward == sbin || sm_chunk_size(pbak->backward) == sm_chunk_size(pbak));

			qchk = sm_next_chunk(pbak);

			if (qchk->head != SM_FENCE_POST_HEAD)
				sm_do_check_in_use_chunk(state, qchk);
		}
	}
}


// Find chunk in a bin. Used in other check functions.
inline static uint8_t sm_bin_find(sm_state_t state, sm_pchunk_t chunk)
{
	size_t size = sm_chunk_size(chunk);

	if (sm_is_small(size))
	{
		sm_bindex_t sidx = sm_small_index(size);
		sm_psbin_t sbin = sm_small_bin_at(state, sidx);

		if (sm_small_map_is_marked(state, sidx))
		{
			sm_pchunk_t pchk = sbin;
			do if (pchk == chunk) return 1;
			while ((pchk = pchk->forward) != sbin);
		}
	}
	else
	{
		sm_bindex_t tidx;

		sm_compute_tree_index(size, tidx);

		if (sm_tree_map_is_marked(state, tidx))
		{
			sm_ptchunk_t tchk = *sm_tree_bin_at(state, tidx);
			size_t sbit = size << sm_left_shift_for_tree_index(tidx);

			while (tchk != 0 && sm_chunk_size(tchk) != size)
			{
				tchk = tchk->child[(sbit >> (SM_SIZE_T_BITSIZE - SM_SIZE_T_ONE)) & UINT64_C(1)];
				sbit <<= 1;
			}

			if (tchk != 0)
			{
				sm_ptchunk_t uchk = tchk;
				do if (uchk == (sm_ptchunk_t)chunk) return 1;
				while ((uchk = uchk->forward) != tchk);
			}
		}
	}

	return 0;
}


// Traverse each chunk and check it; return total.
inline static size_t sm_traverse_and_check(sm_state_t state)
{
	size_t ssum = UINT64_C(0);

	if (sm_is_initialized(state))
	{
		sm_psegment_t sseg = &state->segment;
		ssum += state->top_size + SM_TOP_FOOT_SIZE;

		while (sseg != NULL)
		{
			sm_pchunk_t qchk = sm_align_as_chunk(sseg->base);
			sm_pchunk_t qlst = NULL;

			assert(sm_is_p_in_use(qchk));

			while (sm_segment_holds(sseg, qchk) && qchk != state->top && qchk->head != SM_FENCE_POST_HEAD)
			{
				ssum += sm_chunk_size(qchk);

				if (sm_is_in_use(qchk))
				{
					assert(!sm_bin_find(state, qchk));

					sm_do_check_in_use_chunk(state, qchk);
				}
				else
				{
					assert(qchk == state->dv || sm_bin_find(state, qchk));
					assert(qlst == NULL || sm_is_in_use(qlst));

					sm_do_check_free_chunk(state, qchk);
				}

				qlst = qchk;

				qchk = sm_next_chunk(qchk);
			}

			sseg = sseg->next;
		}
	}

	return ssum;
}


// Check all properties of sm_state_s.
inline static void sm_do_check_allocation_state(sm_state_t state)
{
	sm_bindex_t bidx;
	size_t totl;

	for (bidx = UINT64_C(0); bidx < SM_N_SMALL_BINS; ++bidx) 
		sm_do_check_small_bin(state, bidx);

	for (bidx = UINT64_C(0); bidx < SM_N_TREE_BINS; ++bidx) 
		sm_do_check_tree_bin(state, bidx);

	if (state->dv_size != UINT64_C(0))
	{
		sm_do_check_any_chunk(state, state->dv);

		assert(state->dv_size == sm_chunk_size(state->dv));
		assert(state->dv_size >= SM_MIN_CHUNK_SIZE);
		assert(sm_bin_find(state, state->dv) == NULL);
	}

	if (state->top != NULL)
	{
		sm_do_check_top_chunk(state, state->top);

		assert(state->top_size > UINT64_C(0));
		assert(sm_bin_find(state, state->top) == NULL);
	}

	totl = sm_traverse_and_check(state);

	assert(totl <= state->foot_print);
	assert(state->foot_print <= state->max_foot_print);
}

#endif


#if !SM_NO_ALLOCATION_INFO
inline static struct sm_memory_info_t sm_internal_memory_info(sm_state_t state)
{
	struct sm_memory_info_t imem = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	sm_ensure_initialization();

	if (!SM_PRE_ACTION(state))
	{
		sm_check_allocation_state(state);

		if (sm_is_initialized(state))
		{
			size_t nuse = SM_SIZE_T_ONE;
			size_t muse = state->top_size + SM_TOP_FOOT_SIZE;
			size_t ssum = muse;
			sm_psegment_t pseg = &state->segment;

			while (pseg != 0)
			{
				sm_pchunk_t qchk = sm_align_as_chunk(pseg->base);

				while (sm_segment_holds(pseg, qchk) && qchk != state->top && qchk->head != SM_FENCE_POST_HEAD)
				{
					size_t csiz = sm_chunk_size(qchk);
					ssum += csiz;

					if (!sm_is_in_use(qchk))
					{
						muse += csiz;
						++nuse;
					}

					qchk = sm_next_chunk(qchk);
				}

				pseg = pseg->next;
			}

			imem.arena = ssum;
			imem.count_free_chunks = nuse;
			imem.memory_mapped_space = state->foot_print - ssum;
			imem.max_total_allocated_space = state->max_foot_print;
			imem.total_allocated_space = state->foot_print - muse;
			imem.total_free_space = muse;
			imem.keep_cost = state->top_size;
		}

		SM_POST_ACTION(state);
	}

	return imem;
}
#endif


#if !SM_NO_MALLOC_STATS
inline static sm_allocation_stats_t sm_internal_malloc_stats(sm_state_t state)
{
	sm_allocation_stats_t stat = { UINT64_C(0), UINT64_C(0), UINT64_C(0) };

	sm_ensure_initialization();

	if (!SM_PRE_ACTION(state))
	{
		size_t mxfp = UINT64_C(0);
		size_t fpnr = UINT64_C(0);
		size_t used = UINT64_C(0);

		sm_check_allocation_state(state);

		if (sm_is_initialized(state))
		{
			sm_psegment_t sseg = &state->segment;
			mxfp = state->max_foot_print;
			fpnr = state->foot_print;
			used = fpnr - (state->top_size + SM_TOP_FOOT_SIZE);

			while (sseg != NULL)
			{
				sm_pchunk_t qchk = sm_align_as_chunk(sseg->base);

				while (sm_segment_holds(sseg, qchk) && qchk != state->top && qchk->head != SM_FENCE_POST_HEAD)
				{
					if (!sm_is_in_use(qchk))
						used -= sm_chunk_size(qchk);
					qchk = sm_next_chunk(qchk);
				}

				sseg = sseg->next;
			}
		}

		SM_POST_ACTION(state);

		stat.maximum_system_bytes = mxfp;
		stat.system_bytes = fpnr;
		stat.in_use_bytes = used;
	}

	return stat;
}
#endif


// Operations on Small Bins


// Link a free chunk into a small bin.
#define sm_insert_small_chunk(M, P, S) { sm_bindex_t isci__ = sm_small_index(S); sm_pchunk_t iscb__ = sm_small_bin_at(M, isci__); \
	sm_pchunk_t iscf__ = iscb__; assert(S >= SM_MIN_CHUNK_SIZE); if (!sm_small_map_is_marked(M, isci__)) sm_mark_small_map(M, isci__); \
	else if (sm_runtime_check(sm_is_address_ok(M, iscb__->forward))) iscf__ = iscb__->forward; else SM_CORRUPTION_ERROR_ACTION(M); \
	iscb__->forward = P; iscf__->backward = P; P->forward = iscf__; P->backward = iscb__; }

// Unlink a chunk from a small bin.
#define sm_unlink_small_chunk(M, P, S) { sm_pchunk_t uscf__ = P->forward; sm_pchunk_t uscb__ = P->backward; sm_bindex_t usci__ = sm_small_index(S); \
	assert(P != uscb__); assert(P != uscf__); assert(sm_chunk_size(P) == sm_small_index_to_size(usci__)); \
	if (sm_runtime_check(uscf__ == sm_small_bin_at(M, usci__) || (sm_is_address_ok(M, uscf__) && uscf__->backward == P))) { \
	if (uscb__ == uscf__) sm_clear_small_map(M, usci__); else if (sm_runtime_check(uscb__ == sm_small_bin_at(M, usci__) || (sm_is_address_ok(M, uscb__) && uscb__->forward == P))) \
	{ uscf__->backward = uscb__; uscb__->forward = uscf__; } else SM_CORRUPTION_ERROR_ACTION(M); } else SM_CORRUPTION_ERROR_ACTION(M); }

// Unlink the first chunk from a small bin.
#define sm_unlink_first_small_chunk(M, B, P, I) { sm_pchunk_t ufsf__ = P->forward; assert(P != B); assert(P != ufsf__); \
	assert(sm_chunk_size(P) == sm_small_index_to_size(I)); if (B == ufsf__) sm_clear_small_map(M, I); \
	else if (sm_runtime_check(sm_is_address_ok(M, ufsf__) && ufsf__->backward == P)) { ufsf__->backward = B; B->forward = ufsf__; } \
	else SM_CORRUPTION_ERROR_ACTION(M); }

// Replace dv node, binning the old one. Used only when dv_size is known to be small.
#define sm_replace_dv(M, P, S) { size_t rdvs__ = M->dv_size; assert(sm_is_small(rdvs__));\
	if (rdvs__ != 0) { sm_pchunk_t rdv__ = M->dv; sm_insert_small_chunk(M, rdv__, rdvs__); } \
	M->dv_size = S; M->dv = P; }


// Operations on Trees


// Insert chunk into tree.
inline static void sm_insert_large_chunk(register sm_state_t state, register sm_ptchunk_t chunk, size_t size)
{
	sm_ptbin_t* hptb;
	sm_bindex_t bidx;

	sm_compute_tree_index(size, bidx);
	hptb = sm_tree_bin_at(state, bidx);

	chunk->index = bidx;
	chunk->child[0] = chunk->child[1] = NULL;

	if (!sm_tree_map_is_marked(state, bidx))
	{
		sm_mark_tree_map(state, bidx);

		*hptb = chunk;
		chunk->parent = (sm_ptchunk_t)hptb;
		chunk->forward = chunk->backward = chunk;
	}
	else
	{
		sm_ptchunk_t tchk = *hptb;
		size_t ksiz = size << sm_left_shift_for_tree_index(bidx);

		for (;;)
		{
			if (sm_chunk_size(tchk) != size)
			{
				register sm_ptchunk_t* cchk = &(tchk->child[(ksiz >> (SM_SIZE_T_BITSIZE - SM_SIZE_T_ONE)) & UINT64_C(1)]);
				ksiz <<= 1;

				if (*cchk != 0) tchk = *cchk;
				else if (sm_runtime_check(sm_is_address_ok(state, cchk)))
				{
					*cchk = chunk;
					chunk->parent = tchk;
					chunk->forward = chunk->backward = chunk;

					break;
				}
				else
				{
					SM_CORRUPTION_ERROR_ACTION(state);

					break;
				}
			}
			else
			{
				register sm_ptchunk_t pfwd = tchk->forward;

				if (sm_runtime_check(sm_is_address_ok(state, tchk) && sm_is_address_ok(state, pfwd)))
				{
					tchk->forward = pfwd->backward = chunk;
					chunk->forward = pfwd;
					chunk->backward = tchk;
					chunk->parent = NULL;

					break;
				}
				else
				{
					SM_CORRUPTION_ERROR_ACTION(state);

					break;
				}
			}
		}
	}
}


inline static void sm_unlink_large_chunk(sm_state_t state, sm_ptchunk_t chunk)
{
	sm_ptchunk_t cppt = chunk->parent;
	sm_ptchunk_t rchk;

	if (chunk->backward != chunk)
	{
		sm_ptchunk_t fchk = chunk->forward;

		rchk = chunk->backward;

		if (sm_runtime_check(sm_is_address_ok(state, fchk) && fchk->backward == chunk && rchk->forward == chunk))
		{
			fchk->backward = rchk;
			rchk->forward = fchk;
		}
		else SM_CORRUPTION_ERROR_ACTION(state);
	}
	else
	{
		sm_ptchunk_t* rpck;

		if (((rchk = *(rpck = &(chunk->child[1]))) != NULL) || ((rchk = *(rpck = &(chunk->child[0]))) != NULL))
		{
			sm_ptchunk_t* cpck;

			while ((*(cpck = &(rchk->child[1])) != NULL) || (*(cpck = &(rchk->child[0])) != NULL))
				rchk = *(rpck = cpck);

			if (sm_runtime_check(sm_is_address_ok(state, rpck))) *rpck = NULL;
			else SM_CORRUPTION_ERROR_ACTION(state);
		}
	}

	if (cppt != NULL)
	{
		sm_ptbin_t* hbin = sm_tree_bin_at(state, chunk->index);

		if (chunk == *hbin)
		{
			if ((*hbin = rchk) == NULL)
				sm_clear_tree_map(state, chunk->index);
		}
		else if (sm_runtime_check(sm_is_address_ok(state, cppt)))
		{
			if (cppt->child[0] == chunk) cppt->child[0] = rchk;
			else cppt->child[1] = rchk;
		}
		else SM_CORRUPTION_ERROR_ACTION(state);

		if (rchk != NULL)
		{
			if (sm_runtime_check(sm_is_address_ok(state, rchk)))
			{
				sm_ptchunk_t capt, cbpt;

				rchk->parent = cppt;

				if ((capt = chunk->child[0]) != NULL)
				{
					if (sm_runtime_check(sm_is_address_ok(state, capt)))
					{
						rchk->child[0] = capt;
						capt->parent = rchk;
					}
					else SM_CORRUPTION_ERROR_ACTION(state);
				}

				if ((cbpt = chunk->child[1]) != NULL)
				{
					if (sm_runtime_check(sm_is_address_ok(state, cbpt)))
					{
						rchk->child[1] = cbpt;
						cbpt->parent = rchk;
					}
					else SM_CORRUPTION_ERROR_ACTION(state);
				}
			}
			else SM_CORRUPTION_ERROR_ACTION(state);
		}
	}
}


// Relays to Large vs Small Bin Operations


#define sm_insert_chunk(M, P, S) { if (sm_is_small(S)) { sm_insert_small_chunk(M, P, S); } \
	else { sm_ptchunk_t ictp__ = (sm_ptchunk_t)(P); sm_insert_large_chunk(M, ictp__, S); } }


#define sm_unlink_chunk(M, P, S) { if (sm_is_small(S)) { sm_unlink_small_chunk(M, P, S); } \
	else { sm_ptchunk_t uctp__ = (sm_ptchunk_t)(P); sm_unlink_large_chunk(M, uctp__); } }


// Relays to Internal Calls to Malloc/Free


#if SM_ONLY_MSPACES
#define sm_internal_allocate(M, B) sm_space_allocate(M, B)
#define sm_internal_free(M, P) sm_space_free(M, P)
#else
#if SM_MSPACES
#define sm_internal_allocate(M, B) (((M == sm_gm) ? sm_dl_malloc(B) : sm_space_allocate(M, B)))
#define sm_internal_free(M, P) if (M == sm_gm) sm_dl_free(P); else sm_space_free(M, P);
#else
#define sm_internal_allocate(M, B) sm_dl_malloc(B)
#define sm_internal_free(M, P) sm_dl_free(P)
#endif
#endif


// Direct Memory Mapping Chunks


// Malloc using mmap.
inline static void* sm_map_allocate(sm_state_t state, size_t bytes)
{
	size_t msiz = sm_map_align(bytes + SM_SIX_SIZE_T_SIZES + SM_CHUNK_ALIGN_MASK);

	if (state->foot_print_limit != 0)
	{
		size_t fpsz = state->foot_print + msiz;
		if (fpsz <= state->foot_print || fpsz > state->foot_print_limit)
			return NULL;
	}

	if (msiz > bytes)
	{
		uint8_t* mmpt = (uint8_t*)(SM_CALL_DIRECT_MMAP(msiz));

		if (mmpt != SM_MC_FAIL)
		{
			size_t offs = sm_align_offset(sm_chunk_to_memory(mmpt));
			size_t psiz = msiz - offs - SM_MMAP_FOOT_PAD;
			sm_pchunk_t pchk = (sm_pchunk_t)(mmpt + offs);

			pchk->previous = offs;
			pchk->head = psiz;

			sm_mark_in_use_foot(state, pchk, psiz);
			sm_chunk_plus_offset(pchk, psiz)->head = SM_FENCE_POST_HEAD;
			sm_chunk_plus_offset(pchk, psiz + SM_SIZE_T_SIZE)->head = UINT64_C(0);

			if (state->least_address == NULL || mmpt < state->least_address)
				state->least_address = mmpt;

			if ((state->foot_print += msiz) > state->max_foot_print)
				state->max_foot_print = state->foot_print;

			assert(sm_is_aligned(sm_chunk_to_memory(pchk)));

			sm_check_mapped_chunk(state, pchk);

			return sm_chunk_to_memory(pchk);
		}
	}

	return NULL;
}


// Realloc using memory mapping.
inline static sm_pchunk_t sm_map_resize(sm_state_t state, sm_pchunk_t chunk, size_t bytes, int flags)
{
	size_t osiz = sm_chunk_size(chunk);

	if (sm_is_small(bytes)) return NULL;

	if (osiz >= bytes + SM_SIZE_T_SIZE && (osiz - bytes) <= (sm_m_params__.granularity << 1))
		return chunk;
	else
	{
		size_t offs = chunk->previous;
		size_t oldm = osiz + offs + SM_MMAP_FOOT_PAD;
		size_t newm = sm_map_align(bytes + SM_SIX_SIZE_T_SIZES + SM_CHUNK_ALIGN_MASK);

		uint8_t* cptr = (uint8_t*)SM_CALL_MREMAP((uint8_t*)chunk - offs, oldm, newm, flags);

		if (cptr != SM_MC_FAIL)
		{
			sm_pchunk_t newp = (sm_pchunk_t)(cptr + offs);
			size_t psiz = newm - offs - SM_MMAP_FOOT_PAD;

			newp->head = psiz;

			sm_mark_in_use_foot(state, newp, psiz);
			sm_chunk_plus_offset(newp, psiz)->head = SM_FENCE_POST_HEAD;
			sm_chunk_plus_offset(newp, psiz + SM_SIZE_T_SIZE)->head = UINT64_C(0);

			if (cptr < state->least_address)
				state->least_address = cptr;

			if ((state->foot_print += newm - oldm) > state->max_foot_print)
				state->max_foot_print = state->foot_print;

			sm_check_mapped_chunk(state, newp);

			return newp;
		}
	}

	return NULL;
}


// Memory Space Management


// Initialize top chunk and its size.
inline static void sm_initialize_top_chunk(sm_state_t state, sm_pchunk_t chunk, size_t size)
{
	size_t offs = sm_align_offset(sm_chunk_to_memory(chunk));
	chunk = (sm_pchunk_t)((uint8_t*)chunk + offs);
	size -= offs;
	state->top = chunk;
	state->top_size = size;
	chunk->head = size | SM_P_IN_USE_BIT;
	sm_chunk_plus_offset(chunk, size)->head = SM_TOP_FOOT_SIZE;
	state->trim_check = sm_m_params__.trim_threshold;
}


// Initialize bins for a new memory state that is otherwise zeroed out.
inline static void sm_initialize_bins(sm_state_t state)
{
	sm_bindex_t bidx;

	for (bidx = UINT64_C(0); bidx < SM_N_SMALL_BINS; ++bidx)
	{
		sm_psbin_t bptr = sm_small_bin_at(state, bidx);
		bptr->forward = bptr->backward = bptr;
	}
}

#if SM_PROCEED_ON_ERROR
// Default corruption action.
inline static void sm_reset_on_error(sm_state_t state)
{
	++sm_malloc_corruption_error_count__;

	state->small_map = state->tree_map = UINT64_C(0);
	state->dv_size = state->top_size = UINT64_C(0);
	state->segment.base = NULL;
	state->segment.size = UINT64_C(0);
	state->segment.next = NULL;
	state->top = state->dv = NULL;

	size_t indx;

	for (indx = 0; indx < SM_N_TREE_BINS; ++iindx)
		*sm_tree_bin_at(state, indx) = NULL;

	sm_initialize_bins(state);
}
#endif


// Allocate chunk and prepend remainder with chunk in successor base.
inline static void* sm_prepend_alloc(sm_state_t state, uint8_t* next, uint8_t* previous, size_t bytes)
{
	sm_pchunk_t pptr = sm_align_as_chunk(next);
	sm_pchunk_t oldf = sm_align_as_chunk(previous);
	size_t psiz = (uint8_t*)oldf - (uint8_t*)pptr;
	sm_pchunk_t qptr = sm_chunk_plus_offset(pptr, bytes);
	size_t qsiz = psiz - bytes;

	sm_sets_p_inuse_inuse_chunk(state, pptr, bytes);

	assert((uint8_t*)oldf > (uint8_t*)qptr);
	assert(sm_is_p_in_use(oldf));
	assert(qsiz >= SM_MIN_CHUNK_SIZE);

	if (oldf == state->top)
	{
		size_t tsiz = state->top_size += qsiz;
		state->top = qptr;
		qptr->head = tsiz | SM_P_IN_USE_BIT;
		sm_check_top_chunk(state, qptr);
	}
	else if (oldf == state->dv)
	{
		size_t dsiz = state->dv_size += qsiz;
		state->dv = qptr;
		sm_sets_p_inuse_fchunk(qptr, dsiz);
	}
	else
	{
		if (!sm_is_in_use(oldf))
		{
			size_t nsiz = sm_chunk_size(oldf);
			sm_unlink_chunk(state, oldf, nsiz);
			oldf = sm_chunk_plus_offset(oldf, nsiz);
			qsiz += nsiz;
		}

		sm_set_free_with_p_in_use(qptr, qsiz, oldf);
		sm_insert_chunk(state, qptr, qsiz);
		sm_check_free_chunk(state, qptr);
	}

	sm_check_allocated_chunk(state, sm_chunk_to_memory(pptr), bytes);

	return sm_chunk_to_memory(pptr);
}


// Add a segment to hold a new non-contiguous region.
inline static void sm_add_segment(sm_state_t state, uint8_t* base, size_t size, sm_flag_t mapped)
{
	uint8_t* oldt = (uint8_t*)state->top;
	sm_psegment_t ospc = sm_segment_holding(state, oldt);
	uint8_t* olde = ospc->base + ospc->size;
	size_t ssiz = sm_pad_request(sizeof(struct sm_segment_s));
	uint8_t* praw = olde - (ssiz + SM_FOUR_SIZE_T_SIZES + SM_CHUNK_ALIGN_MASK);
	size_t offs = sm_align_offset(sm_chunk_to_memory(praw));
	uint8_t* aspt = praw + offs;
	uint8_t* cspt = (aspt < (oldt + SM_MIN_CHUNK_SIZE)) ? oldt : aspt;
	sm_pchunk_t spck = (sm_pchunk_t)cspt;
	sm_psegment_t sseg = (sm_psegment_t)(sm_chunk_to_memory(spck));
	sm_pchunk_t tnex = sm_chunk_plus_offset(spck, ssiz);
	sm_pchunk_t pnex = tnex;
	uint64_t nfen = UINT64_C(0);

	sm_initialize_top_chunk(state, (sm_pchunk_t)base, size - SM_TOP_FOOT_SIZE);

	assert(sm_is_aligned(sseg));

	sm_sets_p_inuse_inuse_chunk(state, spck, ssiz);

	*sseg = state->segment;
	state->segment.base = base;
	state->segment.size = size;
	state->segment.flags = mapped;
	state->segment.next = sseg;

	// <TODO WHAT="Verify pnex vs. ptmp">
	for (;;)
	{
		sm_pchunk_t ptmp = sm_chunk_plus_offset(pnex, SM_SIZE_T_SIZE);
		pnex->head = SM_FENCE_POST_HEAD;
		++nfen;

		if ((uint8_t*)(&(ptmp->head)) < olde)
			pnex = ptmp;
		else break;
	}
	// </TODO>

	assert(nfen >= UINT64_C(2));

	if (cspt != oldt)
	{
		sm_pchunk_t qchk = (sm_pchunk_t)oldt;
		size_t psiz = cspt - oldt;
		sm_pchunk_t tnof = sm_chunk_plus_offset(qchk, psiz);
		sm_set_free_with_p_in_use(qchk, psiz, tnof);
		sm_insert_chunk(state, qchk, psiz);
	}

	sm_check_top_chunk(state, state->top);
}


// System Allocation


// Get memory from system using MORECORE or SM_MMAP.
inline static void* sm_system_allocate(sm_state_t state, size_t bytes)
{
	uint8_t* tbas = SM_MC_FAIL;
	size_t tsiz = UINT64_C(0);
	sm_flag_t mflg = 0;
	size_t asiz;

	sm_ensure_initialization();

	if (sm_use_mapping(state) && bytes >= sm_m_params__.mapping_threshold && state->top_size != UINT64_C(0))
	{
		void* pmem = sm_map_allocate(state, bytes);

		if (pmem != NULL)
			return pmem;
	}

	asiz = sm_granularity_align(bytes + SM_SYS_ALLOC_PADDING);

	if (asiz <= bytes) return NULL;

	if (state->foot_print_limit != UINT64_C(0))
	{
		size_t fpsz = state->foot_print + asiz;

		if (fpsz <= state->foot_print || fpsz > state->foot_print_limit)
			return NULL;
	}

	if (SM_MORE_CORE_CONTIGUOUS && !sm_use_non_contiguous(state))
	{
		uint8_t* ptbr = SM_MC_FAIL;
		size_t ssiz = asiz;
		sm_psegment_t sseg = (state->top == NULL) ? NULL : sm_segment_holding(state, (uint8_t*)state->top);

		SM_ACQUIRE_MALLOC_GLOBAL_LOCK();

		if (sseg == 0)
		{
			uint8_t* base = (uint8_t*)SM_CALL_MORE_CORE(0);

			if (base != SM_MC_FAIL)
			{
				size_t fptr;

				if (!sm_is_page_aligned(base))
					ssiz += (sm_page_align((size_t)base) - (size_t)base);

				fptr = state->foot_print + ssiz;

				if (ssiz > bytes && ssiz < SM_HALF_MAX_SIZE_T && (state->foot_print_limit == 0 || (fptr > state->foot_print && fptr <= state->foot_print_limit)) && (ptbr = (char*)(SM_CALL_MORE_CORE(ssiz))) == base)
				{
					tbas = base;
					tsiz = ssiz;
				}
			}
		}
		else
		{
			ssiz = sm_granularity_align(bytes - state->top_size + SM_SYS_ALLOC_PADDING);

			if (ssiz < SM_HALF_MAX_SIZE_T && (ptbr = (uint8_t*)(SM_CALL_MORE_CORE(ssiz))) == sseg->base + sseg->size)
			{
				tbas = ptbr;
				tsiz = ssiz;
			}
		}

		if (tbas == SM_MC_FAIL)
		{
			if (ptbr != SM_MC_FAIL)
			{
				if (ssiz < SM_HALF_MAX_SIZE_T && ssiz < bytes + SM_SYS_ALLOC_PADDING)
				{
					size_t esize = sm_granularity_align(bytes + SM_SYS_ALLOC_PADDING - ssiz);

					if (esize < SM_HALF_MAX_SIZE_T)
					{
						uint8_t* pend = (uint8_t*)SM_CALL_MORE_CORE(esize);

						if (pend != SM_MC_FAIL)
							ssiz += esize;
						else
						{
							SM_CALL_MORE_CORE(-ssiz);
							ptbr = SM_MC_FAIL;
						}
					}
				}
			}

			if (ptbr != SM_MC_FAIL)
			{
				tbas = ptbr;
				tsiz = ssiz;
			}
			else sm_disable_contiguous(state);
		}

		SM_RELEASE_MALLOC_GLOBAL_LOCK();
	}

	if (SM_HAS_MMAP && tbas == SM_MC_FAIL)
	{
		uint8_t* mptr = (uint8_t*)(SM_CALL_MMAP(asiz));

		if (mptr != SM_MC_FAIL)
		{
			tbas = mptr;
			tsiz = asiz;
			mflg = SM_USE_MMAP_BIT;
		}
	}

	if (SM_HAS_MORE_CORE && tbas == SM_MC_FAIL)
	{
		if (asiz < SM_HALF_MAX_SIZE_T)
		{
			uint8_t *ptbr = SM_MC_FAIL, *end = SM_MC_FAIL;

			SM_ACQUIRE_MALLOC_GLOBAL_LOCK();

			ptbr = (uint8_t*)(SM_CALL_MORE_CORE(asiz));
			end = (uint8_t*)(SM_CALL_MORE_CORE(0));

			SM_RELEASE_MALLOC_GLOBAL_LOCK();

			if (ptbr != SM_MC_FAIL && end != SM_MC_FAIL && ptbr < end)
			{
				size_t ssiz = end - ptbr;

				if (ssiz > bytes + SM_TOP_FOOT_SIZE)
				{
					tbas = ptbr;
					tsiz = ssiz;
				}
			}
		}
	}

	if (tbas != SM_MC_FAIL)
	{
		if ((state->foot_print += tsiz) > state->max_foot_print)
			state->max_foot_print = state->foot_print;

		if (!sm_is_initialized(state))
		{
			if (state->least_address == NULL || tbas < state->least_address)
				state->least_address = tbas;

			state->segment.base = tbas;
			state->segment.size = tsiz;
			state->segment.flags = mflg;
			state->magic = sm_m_params__.magic;
			state->release_checks = SM_MAX_RELEASE_CHECK_RATE;

			sm_initialize_bins(state);

#if !SM_ONLY_MSPACES
			if (sm_is_global(state))
				sm_initialize_top_chunk(state, (sm_pchunk_t)tbas, tsiz - SM_TOP_FOOT_SIZE);
			else
#endif
			{
				sm_pchunk_t mnck = sm_next_chunk(sm_memory_to_chunk(state));
				sm_initialize_top_chunk(state, mnck, (size_t)((tbas + tsiz) - (uint8_t*)mnck) - SM_TOP_FOOT_SIZE);
			}
		}
		else
		{
			sm_psegment_t spsg = &state->segment;

			while (spsg != NULL && tbas != spsg->base + spsg->size)
				spsg = (SM_NO_SEGMENT_TRAVERSAL) ? NULL : spsg->next;

			if (spsg != NULL && !sm_is_external_segment(spsg) && (spsg->flags & SM_USE_MMAP_BIT) == mflg && sm_segment_holds(spsg, state->top))
			{
				spsg->size += tsiz;
				sm_initialize_top_chunk(state, state->top, state->top_size + tsiz);
			}
			else
			{
				if (tbas < state->least_address)
					state->least_address = tbas;

				spsg = &state->segment;

				while (spsg != NULL && spsg->base != tbas + tsiz)
					spsg = (SM_NO_SEGMENT_TRAVERSAL) ? NULL : spsg->next;

				if (spsg != NULL && !sm_is_external_segment(spsg) && (spsg->flags & SM_USE_MMAP_BIT) == mflg)
				{
					uint8_t* obas = spsg->base;
					spsg->base = tbas;
					spsg->size += tsiz;

					return sm_prepend_alloc(state, tbas, obas, bytes);
				}
				else sm_add_segment(state, tbas, tsiz, mflg);
			}
		}

		if (bytes < state->top_size)
		{
			size_t rsiz = state->top_size -= bytes;
			sm_pchunk_t pchk = state->top;
			sm_pchunk_t rchk = state->top = sm_chunk_plus_offset(pchk, bytes);

			rchk->head = rsiz | SM_P_IN_USE_BIT;

			sm_sets_p_inuse_inuse_chunk(state, pchk, bytes);
			sm_check_top_chunk(state, state->top);
			sm_check_allocated_chunk(state, sm_chunk_to_memory(pchk), bytes);

			return sm_chunk_to_memory(pchk);
		}
	}

	SM_MALLOC_FAILURE_ACTION;

	return NULL;
}


// System Deallocation


// Unmap and unlink any mmapped segments that don't contain used chunks.
inline static size_t sm_release_unused_segments(sm_state_t state)
{
	size_t nrel = UINT64_C(0);
	int nseg = 0;
	sm_psegment_t pred = &state->segment;
	sm_psegment_t pnex = pred->next;

	while (pnex != NULL)
	{
		uint8_t* base = pnex->base;
		size_t size = pnex->size;
		sm_psegment_t next = pnex->next;
		++nseg;

		if (sm_is_mapped_segment(pnex) && !sm_is_external_segment(pnex))
		{
			sm_pchunk_t ptmp = sm_align_as_chunk(base);
			size_t psiz = sm_chunk_size(ptmp);

			if (!sm_is_in_use(ptmp) && (uint8_t*)ptmp + psiz >= base + size - SM_TOP_FOOT_SIZE)
			{
				sm_ptchunk_t tpck = (sm_ptchunk_t)ptmp;

				assert(sm_segment_holds(pnex, (uint8_t*)pnex));

				if (ptmp == state->dv)
				{
					state->dv = NULL;
					state->dv_size = UINT64_C(0);
				}
				else sm_unlink_large_chunk(state, tpck);

				if (SM_CALL_MUNMAP(base, size) == 0)
				{
					nrel += size;
					state->foot_print -= size;
					pnex = pred;
					pnex->next = next;
				}
				else sm_insert_large_chunk(state, tpck, psiz);
			}
		}

		if (SM_NO_SEGMENT_TRAVERSAL) break;

		pred = pnex;
		pnex = next;
	}

	state->release_checks = (((size_t)nseg > (size_t)SM_MAX_RELEASE_CHECK_RATE) ? (size_t)nseg : (size_t)SM_MAX_RELEASE_CHECK_RATE);

	return nrel;
}


inline static uint8_t sm_system_trim(sm_state_t state, size_t padding)
{
	size_t nrel = UINT64_C(0);

	sm_ensure_initialization();

	if (padding < SM_MAX_REQUEST && sm_is_initialized(state))
	{
		padding += SM_TOP_FOOT_SIZE;

		if (state->top_size > padding)
		{
			size_t unit = sm_m_params__.granularity;
			size_t extr = ((state->top_size - padding + (unit - SM_SIZE_T_ONE)) / unit - SM_SIZE_T_ONE) * unit;
			sm_psegment_t sptr = sm_segment_holding(state, (uint8_t*)state->top);

			if (!sm_is_external_segment(sptr))
			{
				if (sm_is_mapped_segment(sptr))
				{
					if (SM_HAS_MMAP && sptr->size >= extr && !sm_has_segment_link(state, sptr))
					{
						size_t news = sptr->size - extr;

						if ((SM_CALL_MREMAP(sptr->base, sptr->size, news, 0) != SM_M_FAIL) || (SM_CALL_MUNMAP(sptr->base + news, extr) == 0))
							nrel = extr;
					}
				}
				else if (SM_HAS_MORE_CORE)
				{
					if (extr >= SM_HALF_MAX_SIZE_T)
						extr = (SM_HALF_MAX_SIZE_T)+SM_SIZE_T_ONE - unit;

					SM_ACQUIRE_MALLOC_GLOBAL_LOCK();

					{
						uint8_t* pobr = (uint8_t*)(SM_CALL_MORE_CORE(0));

						if (pobr == sptr->base + sptr->size)
						{
							uint8_t* rbrc = (uint8_t*)(SM_CALL_MORE_CORE(-extr));
							uint8_t* nbrc = (uint8_t*)(SM_CALL_MORE_CORE(0));

							if (rbrc != SM_MC_FAIL && nbrc < pobr)
								nrel = pobr - nbrc;
						}
					}

					SM_RELEASE_MALLOC_GLOBAL_LOCK();
				}
			}

			if (nrel != UINT64_C(0))
			{
				sptr->size -= nrel;
				state->foot_print -= nrel;

				sm_initialize_top_chunk(state, state->top, state->top_size - nrel);
				sm_check_top_chunk(state, state->top);
			}
		}

		if (SM_HAS_MMAP)
			nrel += sm_release_unused_segments(state);

		if (nrel == UINT64_C(0) && state->top_size > state->trim_check)
			state->trim_check = SM_MAX_SIZE_T;
	}

	return (nrel != UINT64_C(0)) ? 1 : 0;
}


// Consolidate and bin a chunk. Differs from exported versions of free mainly in that the chunk need not be marked as in-use.
inline static void sm_dispose_chunk(sm_state_t state, sm_pchunk_t chunk, size_t size)
{
	sm_pchunk_t next = sm_chunk_plus_offset(chunk, size);

	if (!sm_is_p_in_use(chunk))
	{
		sm_pchunk_t prev;
		size_t prvs = chunk->previous;

		if (sm_is_mapped(chunk))
		{
			size += prvs + SM_MMAP_FOOT_PAD;

			if (SM_CALL_MUNMAP((char*)chunk - prvs, size) == 0)
				state->foot_print -= size;

			return;
		}

		prev = sm_chunk_minus_offset(chunk, prvs);
		size += prvs;
		chunk = prev;

		if (sm_runtime_check(sm_is_address_ok(state, prev)))
		{
			if (chunk != state->dv) { sm_unlink_chunk(state, chunk, prvs); }
			else if ((next->head & SM_IN_USE_BITS) == SM_IN_USE_BITS)
			{
				state->dv_size = size;
				sm_set_free_with_p_in_use(chunk, size, next);

				return;
			}
		}
		else
		{
			SM_CORRUPTION_ERROR_ACTION(state);

			return;
		}
	}

	if (sm_runtime_check(sm_is_address_ok(state, next)))
	{
		if (!sm_chunk_in_use(next))
		{
			if (next == state->top)
			{
				size_t tsiz = state->top_size += size;
				state->top = chunk;
				chunk->head = tsiz | SM_P_IN_USE_BIT;

				if (chunk == state->dv)
				{
					state->dv = NULL;
					state->dv_size = UINT64_C(0);
				}

				return;
			}
			else if (next == state->dv)
			{
				size_t dsiz = state->dv_size += size;
				state->dv = chunk;
				sm_sets_p_inuse_fchunk(chunk, dsiz);

				return;
			}
			else
			{
				size_t nsiz = sm_chunk_size(next);
				size += nsiz;

				sm_unlink_chunk(state, next, nsiz);
				sm_sets_p_inuse_fchunk(chunk, size);

				if (chunk == state->dv)
				{
					state->dv_size = size;

					return;
				}
			}
		}
		else
		{
			sm_set_free_with_p_in_use(chunk, size, next);
		}

		sm_insert_chunk(state, chunk, size);
	}
	else SM_CORRUPTION_ERROR_ACTION(state);
}


// Malloc


// Allocate a large request from the best fitting chunk in a tree bin.
inline static void* sm_tree_allocate_large(sm_state_t state, size_t bytes)
{
	sm_ptchunk_t vptr = NULL;
	size_t rsiz = -bytes;
	sm_ptchunk_t tptr;
	sm_bindex_t bidx;

	sm_compute_tree_index(bytes, bidx);

	if ((tptr = *sm_tree_bin_at(state, bidx)) != NULL)
	{
		size_t bsiz = bytes << sm_left_shift_for_tree_index(bidx);
		sm_ptchunk_t prst = NULL;

		for (;;)
		{
			sm_ptchunk_t rtpt;
			size_t trem = sm_chunk_size(tptr) - bytes;

			if (trem < rsiz)
			{
				vptr = tptr;

				if ((rsiz = trem) == UINT64_C(0))
					break;
			}

			rtpt = tptr->child[1];
			tptr = tptr->child[(bsiz >> (SM_SIZE_T_BITSIZE - SM_SIZE_T_ONE)) & UINT64_C(1)];

			if (rtpt != NULL && rtpt != tptr)
				prst = rtpt;

			if (tptr == NULL)
			{
				tptr = prst;
				break;
			}

			bsiz <<= 1;
		}
	}

	if (tptr == NULL && vptr == NULL)
	{
		sm_bin_map_t lbit = sm_left_bits(sm_index_to_bit(bidx)) & state->tree_map;

		if (lbit != UINT64_C(0))
		{
			sm_bindex_t indx;
			sm_bin_map_t bitl = sm_least_bit(lbit);

			sm_compute_bit_to_index(bitl, indx);
			tptr = *sm_tree_bin_at(state, indx);
		}
	}

	while (tptr != NULL)
	{
		size_t trem = sm_chunk_size(tptr) - bytes;

		if (trem < rsiz)
		{
			rsiz = trem;
			vptr = tptr;
		}

		tptr = sm_left_most_child(tptr);
	}

	if (vptr != NULL && rsiz < (size_t)(state->dv_size - bytes))
	{
		if (sm_runtime_check(sm_is_address_ok(state, vptr)))
		{
			sm_pchunk_t roff = sm_chunk_plus_offset(vptr, bytes);

			assert(sm_chunk_size(vptr) == rsiz + bytes);

			if (sm_runtime_check(sm_ok_next(vptr, roff)))
			{
				sm_unlink_large_chunk(state, vptr);

				if (rsiz < SM_MIN_CHUNK_SIZE)
					sm_set_in_use_and_p_in_use(state, vptr, (rsiz + bytes));
				else
				{
					sm_sets_p_inuse_inuse_chunk(state, vptr, bytes);
					sm_sets_p_inuse_fchunk(roff, rsiz);
					sm_insert_chunk(state, roff, rsiz);
				}

				return sm_chunk_to_memory(vptr);
			}
		}

		SM_CORRUPTION_ERROR_ACTION(state);
	}

	return NULL;
}


// Allocate a small request from the best fitting chunk in a tree bin.
inline static void* sm_tree_allocate_small(sm_state_t state, size_t bytes)
{
	sm_ptchunk_t tptr, vptr;
	size_t rsiz;
	sm_bindex_t bidx;
	sm_bin_map_t lbit = sm_least_bit(state->tree_map);

	sm_compute_bit_to_index(lbit, bidx);

	vptr = tptr = *sm_tree_bin_at(state, bidx);
	rsiz = sm_chunk_size(tptr) - bytes;

	while ((tptr = sm_left_most_child(tptr)) != NULL)
	{
		size_t trem = sm_chunk_size(tptr) - bytes;

		if (trem < rsiz)
		{
			rsiz = trem;
			vptr = tptr;
		}
	}

	if (sm_runtime_check(sm_is_address_ok(state, vptr)))
	{
		sm_pchunk_t rptr = sm_chunk_plus_offset(vptr, bytes);

		assert(sm_chunk_size(vptr) == rsiz + bytes);

		if (sm_runtime_check(sm_ok_next(vptr, rptr)))
		{
			sm_unlink_large_chunk(state, vptr);

			if (rsiz < SM_MIN_CHUNK_SIZE)
				sm_set_in_use_and_p_in_use(state, vptr, (rsiz + bytes));
			else
			{
				sm_sets_p_inuse_inuse_chunk(state, vptr, bytes);
				sm_sets_p_inuse_fchunk(rptr, rsiz);
				sm_replace_dv(state, rptr, rsiz);
			}

			return sm_chunk_to_memory(vptr);
		}
	}

	SM_CORRUPTION_ERROR_ACTION(state);

	return NULL;
}


// Internal Support for Realloc, Memalign


// Try to realloc; only in-place unless can_move true.
inline static sm_pchunk_t sm_try_reallocate_chunk(sm_state_t state, sm_pchunk_t chunk, size_t bytes, uint8_t movable)
{
	sm_pchunk_t newp = NULL;
	size_t osiz = sm_chunk_size(chunk);
	sm_pchunk_t next = sm_chunk_plus_offset(chunk, osiz);

	if (sm_runtime_check(sm_is_address_ok(state, chunk) && sm_is_in_use_ok(chunk) && sm_ok_next(chunk, next) && sm_ok_p_in_use(next)))
	{
		if (sm_is_mapped(chunk))
			newp = sm_map_resize(state, chunk, bytes, movable);
		else if (osiz >= bytes)
		{
			size_t rsiz = osiz - bytes;

			if (rsiz >= SM_MIN_CHUNK_SIZE)
			{
				sm_pchunk_t rtmp = sm_chunk_plus_offset(chunk, bytes);

				sm_set_in_use(state, chunk, bytes);
				sm_set_in_use(state, rtmp, rsiz);
				sm_dispose_chunk(state, rtmp, rsiz);
			}

			newp = chunk;
		}
		else if (next == state->top)
		{
			if (osiz + state->top_size > bytes)
			{
				size_t nsiz = osiz + state->top_size;
				size_t ntsz = nsiz - bytes;
				sm_pchunk_t tnew = sm_chunk_plus_offset(chunk, bytes);

				sm_set_in_use(state, chunk, bytes);

				tnew->head = ntsz | SM_P_IN_USE_BIT;
				state->top = tnew;
				state->top_size = ntsz;
				newp = chunk;
			}
		}
		else if (next == state->dv)
		{
			size_t dvss = state->dv_size;

			if (osiz + dvss >= bytes)
			{
				size_t dsiz = osiz + dvss - bytes;

				if (dsiz >= SM_MIN_CHUNK_SIZE)
				{
					sm_pchunk_t rtmp = sm_chunk_plus_offset(chunk, bytes);
					sm_pchunk_t noff = sm_chunk_plus_offset(rtmp, dsiz);

					sm_set_in_use(state, chunk, bytes);
					sm_sets_p_inuse_fchunk(rtmp, dsiz);
					sm_clear_chunk_in_use(noff);

					state->dv_size = dsiz;
					state->dv = rtmp;
				}
				else
				{
					size_t nsiz = osiz + dvss;

					sm_set_in_use(state, chunk, nsiz);

					state->dv_size = UINT64_C(0);
					state->dv = NULL;
				}

				newp = chunk;
			}
		}
		else if (!sm_chunk_in_use(next))
		{
			size_t nxts = sm_chunk_size(next);

			if (osiz + nxts >= bytes)
			{
				size_t rsiz = osiz + nxts - bytes;

				sm_unlink_chunk(state, next, nxts);

				if (rsiz < SM_MIN_CHUNK_SIZE)
				{
					size_t nsiz = osiz + nxts;

					sm_set_in_use(state, chunk, nsiz);
				}
				else
				{
					sm_pchunk_t rtmp = sm_chunk_plus_offset(chunk, bytes);

					sm_set_in_use(state, chunk, bytes);
					sm_set_in_use(state, rtmp, rsiz);

					sm_dispose_chunk(state, rtmp, rsiz);
				}

				newp = chunk;
			}
		}
	}
	else
	{
		SM_USAGE_ERROR_ACTION(state, sm_chunk_to_memory(chunk));
	}

	return newp;
}


inline static void* sm_internal_memory_align(sm_state_t state, size_t alignment, size_t bytes)
{
	void* pmem = NULL;

	if (alignment < SM_MIN_CHUNK_SIZE)
		alignment = SM_MIN_CHUNK_SIZE;

	if ((alignment & (alignment - SM_SIZE_T_ONE)) != UINT64_C(0))
	{
		size_t alin = SM_MALLOC_ALIGNMENT << 1;

		while (alin < alignment) alin <<= 1;

		alignment = alin;
	}

	if (bytes >= SM_MAX_REQUEST - alignment)
	{
		if (state != NULL)
		{
			SM_MALLOC_FAILURE_ACTION;
		}
	}
	else
	{
		size_t bcnt = sm_request_to_size(bytes);
		size_t reqs = bcnt + alignment + SM_MIN_CHUNK_SIZE - SM_CHUNK_OVERHEAD;

		pmem = sm_internal_allocate(state, reqs);

		if (pmem != NULL)
		{
			sm_pchunk_t ptmp = sm_memory_to_chunk(pmem);

			if (SM_PRE_ACTION(state))
				return NULL;

			if ((((size_t)(pmem)) & (alignment - UINT64_C(1))) != UINT64_C(0))
			{
				uint8_t* brpt = (uint8_t*)sm_memory_to_chunk((size_t)(((size_t)((uint8_t*)pmem + alignment - SM_SIZE_T_ONE)) & -alignment));
				uint8_t* ppos = ((size_t)(brpt - (uint8_t*)(ptmp)) >= SM_MIN_CHUNK_SIZE) ? brpt : brpt + alignment;
				sm_pchunk_t newp = (sm_pchunk_t)ppos;
				size_t lsiz = ppos - (uint8_t*)(ptmp);
				size_t nsiz = sm_chunk_size(ptmp) - lsiz;

				if (sm_is_mapped(ptmp))
				{
					newp->previous = ptmp->previous + lsiz;
					newp->head = nsiz;
				}
				else
				{
					sm_set_in_use(state, newp, nsiz);
					sm_set_in_use(state, ptmp, lsiz);
					sm_dispose_chunk(state, ptmp, lsiz);
				}

				ptmp = newp;
			}

			if (!sm_is_mapped(ptmp))
			{
				size_t size = sm_chunk_size(ptmp);

				if (size > bcnt + SM_MIN_CHUNK_SIZE)
				{
					size_t rems = size - bcnt;
					sm_pchunk_t remp = sm_chunk_plus_offset(ptmp, bcnt);

					sm_set_in_use(state, ptmp, bcnt);
					sm_set_in_use(state, remp, rems);

					sm_dispose_chunk(state, remp, rems);
				}
			}

			pmem = sm_chunk_to_memory(ptmp);

			assert(sm_chunk_size(ptmp) >= bcnt);
			assert(((size_t)pmem & (alignment - UINT64_C(1))) == UINT64_C(0));

			sm_check_in_use_chunk(state, ptmp);

			SM_POST_ACTION(state);
		}
	}

	return pmem;
}


// Common support for independent routines, handling all of the combinations that can result.
// The opts arg has: bit 0 set if all elements are same size (using sizes[0]) and bit 1 set if elements should be zeroed
inline static void** sm_independent_allocate(sm_state_t state, size_t count, size_t* sizes, uint8_t options, void** chunks)
{
	size_t indx, size, esiz, csiz, asiz, rsiz;
	void *pmem, **pary;
	sm_pchunk_t pmck, pach;
	sm_flag_t mmon;

	sm_ensure_initialization();

	if (chunks != NULL)
	{
		if (count == UINT64_C(0))
			return chunks;

		pary = chunks;
		asiz = UINT64_C(0);
	}
	else
	{
		if (count == UINT64_C(0))
			return (void**)sm_internal_allocate(state, UINT64_C(0));

		pary = NULL;
		asiz = sm_request_to_size(count * (sizeof(void*)));
	}

	if (options & 1)
	{
		esiz = sm_request_to_size(*sizes);
		csiz = count * esiz;
	}
	else
	{
		esiz = csiz = UINT64_C(0);

		for (indx = UINT64_C(0); indx != count; ++indx)
			csiz += sm_request_to_size(sizes[indx]);
	}

	size = csiz + asiz;

	mmon = sm_use_mapping(state);
	sm_disable_mapping(state);

	pmem = sm_internal_allocate(state, size - SM_CHUNK_OVERHEAD);

	if (mmon) sm_enable_mapping(state);
	if (pmem == NULL) return NULL;

	if (SM_PRE_ACTION(state)) return NULL;

	pmck = sm_memory_to_chunk(pmem);
	rsiz = sm_chunk_size(pmck);

	assert(!sm_is_mapped(pmck));

	if (options & 2)
	{
		register uint8_t* mssp = (uint8_t*)pmem;
		register size_t mssz = rsiz - SM_SIZE_T_SIZE - asiz;
		while (mssz-- > UINT64_C(0)) *mssp++ = 0;
	}

	if (pary == NULL)
	{
		size_t arcs;
		pach = sm_chunk_plus_offset(pmck, csiz);
		arcs = rsiz - csiz;
		pary = (void**)(sm_chunk_to_memory(pach));
		sm_sets_p_inuse_inuse_chunk(state, pach, arcs);
		rsiz = csiz;
	}

	for (indx = UINT64_C(0); ; ++indx)
	{
		pary[indx] = sm_chunk_to_memory(pmck);

		if (indx != count - UINT64_C(1))
		{
			if (esiz != UINT64_C(0))
				size = esiz;
			else size = sm_request_to_size(sizes[indx]);

			rsiz -= size;

			sm_sets_p_inuse_inuse_chunk(state, pmck, size);
			pmck = sm_chunk_plus_offset(pmck, size);
		}
		else
		{
			sm_sets_p_inuse_inuse_chunk(state, pmck, rsiz);
			break;
		}
	}

#if DEBUG
	if (pary != chunks)
	{
		if (esiz != UINT64_C(0))
			assert(rsiz == esiz);
		else assert(rsiz == sm_request_to_size(sizes[indx]));

		sm_check_in_use_chunk(state, sm_memory_to_chunk(pary));
	}

	for (indx = UINT64_C(0); indx != count; ++indx)
		sm_check_in_use_chunk(state, sm_memory_to_chunk(pary[indx]));
#endif

	SM_POST_ACTION(state);

	return pary;
}


// Try to free all pointers in the given array.
inline static size_t sm_internal_bulk_free(sm_state_t state, void** array, size_t elements)
{
	size_t unfr = UINT64_C(0);

	if (!SM_PRE_ACTION(state))
	{
		void** aptr;
		void** fenc = &(array[elements]);

		for (aptr = array; aptr != fenc; ++aptr)
		{
			void* pmem = *aptr;

			if (pmem != NULL)
			{
				sm_pchunk_t pchk = sm_memory_to_chunk(pmem);
				size_t psiz = sm_chunk_size(pchk);

#if SM_FOOTERS
				if (sm_get_memory_state_for(pchk) != state)
				{
					++unfr;
					continue;
				}
#endif

				sm_check_in_use_chunk(state, pchk);

				*aptr = NULL;

				if (sm_runtime_check(sm_is_address_ok(state, pchk) && sm_is_in_use_ok(pchk)))
				{
					void** bptr = aptr + UINT64_C(1);
					sm_pchunk_t next = sm_next_chunk(pchk);

					if (bptr != fenc && *bptr == sm_chunk_to_memory(next))
					{
						size_t nsiz = sm_chunk_size(next) + psiz;
						sm_set_in_use(state, pchk, nsiz);
						*bptr = sm_chunk_to_memory(pchk);
					}
					else sm_dispose_chunk(state, pchk, psiz);
				}
				else
				{
					SM_CORRUPTION_ERROR_ACTION(state);

					break;
				}
			}
		}

		if (sm_should_trim(state, state->top_size))
			sm_system_trim(state, UINT64_C(0));

		SM_POST_ACTION(state);
	}

	return unfr;
}


// Traversal.
#if SM_MALLOC_INSPECT_ALL
inline static void sm_internal_inspect_all(sm_state_t state, void(*visitor)(void* start, void* end, size_t used, void* argument), void* argument)
{
	if (sm_is_initialized(state))
	{
		sm_pchunk_t ptop = state->top;
		sm_psegment_t sptr;

		for (sptr = &state->segment; sptr != NULL; sptr = sptr->next)
		{
			sm_pchunk_t qptr = sm_align_as_chunk(sptr->base);

			while (sm_segment_holds(sptr, qptr) && qptr->head != SM_FENCE_POST_HEAD)
			{
				sm_pchunk_t next = sm_next_chunk(qptr);

				size_t used, csiz = sm_chunk_size(qptr);
				void* strt;

				if (sm_is_in_use(qptr))
				{
					used = csiz - SM_CHUNK_OVERHEAD;
					strt = sm_chunk_to_memory(qptr);
				}
				else
				{
					used = UINT64_C(0);

					if (sm_is_small(csiz))
						strt = (void*)((uint8_t*)qptr + sizeof(struct sm_chunk_s));
					else strt = (void*)((uint8_t*)qptr + sizeof(struct sm_tchunk_s));
				}

				if (strt < (void*)next)
					visitor(strt, next, used, argument);

				if (qptr == ptop)
					break;

				qptr = next;
			}
		}
	}
}
#endif


inline static void* sm_memory_copy(void *restrict p, const void *restrict q, size_t n)
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


// User Memory Spaces


inline static sm_state_t sm_initialize_user_memory_state(uint8_t* base, size_t size)
{
	size_t msiz = sm_pad_request(sizeof(struct sm_state_s));
	sm_pchunk_t mnpt, mspt = sm_align_as_chunk(base);
	sm_state_t state = (sm_state_t)(sm_chunk_to_memory(mspt));

	register uint8_t* mssp = (uint8_t*)state;
	register size_t msnb = msiz;
	while (msnb-- > UINT64_C(0)) *mssp++ = 0;

	SM_INITIAL_LOCK(&state->mutex);

	mspt->head = (msiz | SM_IN_USE_BITS);

	state->segment.base = state->least_address = base;
	state->segment.size = state->foot_print = state->max_foot_print = size;
	state->magic = sm_m_params__.magic;
	state->release_checks = SM_MAX_RELEASE_CHECK_RATE;
	state->flags = sm_m_params__.default_flags;
	state->external_pointer = NULL;
	state->external_size = UINT64_C(0);

	sm_disable_contiguous(state);
	sm_initialize_bins(state);
	mnpt = sm_next_chunk(sm_memory_to_chunk(state));
	sm_initialize_top_chunk(state, mnpt, (size_t)((base + size) - (uint8_t*)mnpt) - SM_TOP_FOOT_SIZE);
	sm_check_top_chunk(state, state->top);

	return state;
}


exported sm_space_t callconv sm_create_space(size_t capacity, uint8_t locked)
{
	sm_state_t space = NULL;

	sm_ensure_initialization();

	size_t msiz = sm_pad_request(sizeof(struct sm_state_s));

	if (capacity < (size_t)-(msiz + SM_TOP_FOOT_SIZE + sm_m_params__.page_size))
	{
		size_t rsiz = ((capacity == 0) ? sm_m_params__.granularity : (capacity + SM_TOP_FOOT_SIZE + msiz));
		size_t tsiz = sm_granularity_align(rsiz);
		uint8_t* tbas = (uint8_t*)(SM_CALL_MMAP(tsiz));

		if (tbas != SM_MC_FAIL)
		{
			space = sm_initialize_user_memory_state(tbas, tsiz);
			space->segment.flags = SM_USE_MMAP_BIT;
			sm_set_locked(space, locked);
		}
	}

	return (sm_space_t)space;
}


sm_space_t sm_create_space_with_base(void* base, size_t capacity, uint8_t locked)
{
	sm_state_t m = NULL;

	sm_ensure_initialization();

	size_t msiz = sm_pad_request(sizeof(struct sm_state_s));

	if (capacity > msiz + SM_TOP_FOOT_SIZE && capacity < (size_t)-(msiz + SM_TOP_FOOT_SIZE + sm_m_params__.page_size))
	{
		m = sm_initialize_user_memory_state((uint8_t*)base, capacity);
		m->segment.flags = SM_EXTERN_BIT;
		sm_set_locked(m, locked);
	}

	return (sm_space_t)m;
}


exported uint8_t callconv sm_space_track_large_chunks(sm_space_t space, uint8_t enable)
{
	uint8_t retv = 0;
	sm_state_t mspt = (sm_state_t)space;

	if (!SM_PRE_ACTION(mspt))
	{
		if (!sm_use_mapping(mspt))
			retv = 1;

		if (!enable)
			sm_enable_mapping(mspt);
		else sm_disable_mapping(mspt);

		SM_POST_ACTION(mspt);
	}

	return retv;
}


exported size_t callconv sm_destroy_space(sm_space_t space)
{
	size_t free = UINT64_C(0);
	sm_state_t mspt = (sm_state_t)space;

	if (sm_is_magic_ok(mspt))
	{
		sm_psegment_t sptr = &mspt->segment;
		SM_DESTROY_LOCK(&mspt->mutex);

		while (sptr != NULL)
		{
			uint8_t* base = sptr->base;
			size_t size = sptr->size;
			sm_flag_t flag = sptr->flags;
			sptr = sptr->next;

			if ((flag & SM_USE_MMAP_BIT) && !(flag & SM_EXTERN_BIT) && SM_CALL_MUNMAP(base, size) == 0)
				free += size;
		}
	}
	else
	{
		SM_USAGE_ERROR_ACTION(mspt, mspt);
	}

	return free;
}


exported void* callconv sm_space_allocate(sm_space_t space, size_t bytes)
{
	sm_state_t mspt = (sm_state_t)space;

	if (!sm_is_magic_ok(mspt))
	{
		SM_USAGE_ERROR_ACTION(mspt, mspt);
		return NULL;
	}

	if (!SM_PRE_ACTION(mspt))
	{
		void* pmem;
		size_t bcnt;

		if (bytes <= SM_MAX_SMALL_REQUEST)
		{
			bcnt = (bytes < SM_MIN_REQUEST) ? SM_MIN_CHUNK_SIZE : sm_pad_request(bytes);

			sm_bindex_t indx = sm_small_index(bcnt);
			sm_bin_map_t sbit = mspt->small_map >> indx;

			if ((sbit & UINT64_C(3)) != UINT64_C(0))
			{
				indx += ~sbit & UINT64_C(1);

				sm_pchunk_t bptr = sm_small_bin_at(mspt, indx);
				sm_pchunk_t pptr = bptr->forward;

				assert(sm_chunk_size(pptr) == sm_small_index_to_size(indx));

				sm_unlink_first_small_chunk(mspt, bptr, pptr, indx);
				sm_set_in_use_and_p_in_use(mspt, pptr, sm_small_index_to_size(indx));

				pmem = sm_chunk_to_memory(pptr);

				sm_check_allocated_chunk(mspt, pmem, bcnt);

				goto LOC_POST_ACTION;
			}
			else if (bcnt > mspt->dv_size)
			{
				if (sbit != UINT64_C(0))
				{
					sm_bin_map_t lfbt = (sbit << indx) & sm_left_bits(sm_index_to_bit(indx));
					sm_bin_map_t lbit = sm_least_bit(lfbt);

					sm_bindex_t i;

					sm_compute_bit_to_index(lbit, i);

					sm_pchunk_t bptr = sm_small_bin_at(mspt, i);
					sm_pchunk_t pptr = bptr->forward;

					assert(sm_chunk_size(pptr) == sm_small_index_to_size(i));

					sm_unlink_first_small_chunk(mspt, bptr, pptr, i);
					size_t rsiz = sm_small_index_to_size(i) - bcnt;

					if (SM_SIZE_T_SIZE != UINT64_C(4) && rsiz < SM_MIN_CHUNK_SIZE)
						sm_set_in_use_and_p_in_use(mspt, pptr, sm_small_index_to_size(i));
					else
					{
						sm_sets_p_inuse_inuse_chunk(mspt, pptr, bcnt);
						sm_pchunk_t rptr = sm_chunk_plus_offset(pptr, bcnt);
						sm_sets_p_inuse_fchunk(rptr, rsiz);
						sm_replace_dv(mspt, rptr, rsiz);
					}

					pmem = sm_chunk_to_memory(pptr);
					sm_check_allocated_chunk(mspt, pmem, bcnt);

					goto LOC_POST_ACTION;
				}
				else if (mspt->tree_map != UINT64_C(0) && (pmem = sm_tree_allocate_small(mspt, bcnt)) != NULL)
				{
					sm_check_allocated_chunk(mspt, pmem, bcnt);

					goto LOC_POST_ACTION;
				}
			}
		}
		else if (bytes >= SM_MAX_REQUEST)
			bcnt = SM_MAX_SIZE_T;
		else
		{
			bcnt = sm_pad_request(bytes);

			if (mspt->tree_map != UINT64_C(0) && (pmem = sm_tree_allocate_large(mspt, bcnt)) != NULL)
			{
				sm_check_allocated_chunk(mspt, pmem, bcnt);

				goto LOC_POST_ACTION;
			}
		}

		if (bcnt <= mspt->dv_size)
		{
			size_t rsiz = mspt->dv_size - bcnt;
			sm_pchunk_t pptr = mspt->dv;

			if (rsiz >= SM_MIN_CHUNK_SIZE)
			{
				sm_pchunk_t rptr = mspt->dv = sm_chunk_plus_offset(pptr, bcnt);
				mspt->dv_size = rsiz;
				sm_sets_p_inuse_fchunk(rptr, rsiz);
				sm_sets_p_inuse_inuse_chunk(mspt, pptr, bcnt);
			}
			else
			{
				size_t dvsz = mspt->dv_size;
				mspt->dv_size = UINT64_C(0);
				mspt->dv = NULL;
				sm_set_in_use_and_p_in_use(mspt, pptr, dvsz);
			}

			pmem = sm_chunk_to_memory(pptr);
			sm_check_allocated_chunk(mspt, pmem, bcnt);

			goto LOC_POST_ACTION;
		}
		else if (bcnt < mspt->top_size)
		{
			size_t rsiz = mspt->top_size -= bcnt;
			sm_pchunk_t pptr = mspt->top;
			sm_pchunk_t rptr = mspt->top = sm_chunk_plus_offset(pptr, bcnt);

			rptr->head = rsiz | SM_P_IN_USE_BIT;
			sm_sets_p_inuse_inuse_chunk(mspt, pptr, bcnt);
			pmem = sm_chunk_to_memory(pptr);
			sm_check_top_chunk(mspt, mspt->top);
			sm_check_allocated_chunk(mspt, pmem, bcnt);

			goto LOC_POST_ACTION;
		}

		pmem = sm_system_allocate(mspt, bcnt);

	LOC_POST_ACTION:

		SM_POST_ACTION(mspt);

		return pmem;
	}

	return NULL;
}


exported void callconv sm_space_free(sm_space_t space, void* memory)
{
	if (memory != NULL)
	{
		sm_pchunk_t pchk = sm_memory_to_chunk(memory);

#if SM_FOOTERS
		sm_state_t fmst = sm_get_memory_state_for(pchk);
#else
		sm_state_t fmst = (sm_state_t)space;
#endif

		if (!sm_is_magic_ok(fmst))
		{
			SM_USAGE_ERROR_ACTION(fmst, pchk);

			return;
		}

		if (!SM_PRE_ACTION(fmst))
		{
			sm_check_in_use_chunk(fmst, pchk);

			if (sm_runtime_check(sm_is_address_ok(fmst, pchk) && sm_is_in_use_ok(pchk)))
			{
				size_t psiz = sm_chunk_size(pchk);
				sm_pchunk_t next = sm_chunk_plus_offset(pchk, psiz);

				if (!sm_is_p_in_use(pchk))
				{
					size_t prvs = pchk->previous;

					if (sm_is_mapped(pchk))
					{
						psiz += prvs + SM_MMAP_FOOT_PAD;

						if (SM_CALL_MUNMAP((char*)pchk - prvs, psiz) == 0)
							fmst->foot_print -= psiz;

						goto LOC_POST_ACTION;
					}
					else
					{
						sm_pchunk_t prev = sm_chunk_minus_offset(pchk, prvs);

						psiz += prvs;
						pchk = prev;

						if (sm_runtime_check(sm_is_address_ok(fmst, prev)))
						{
							if (pchk != fmst->dv)
							{
								sm_unlink_chunk(fmst, pchk, prvs);
							}
							else if ((next->head & SM_IN_USE_BITS) == SM_IN_USE_BITS)
							{
								fmst->dv_size = psiz;
								sm_set_free_with_p_in_use(pchk, psiz, next);

								goto LOC_POST_ACTION;
							}
						}
						else goto LOC_ERROR_ACTION;
					}
				}

				if (sm_runtime_check(sm_ok_next(pchk, next) && sm_ok_p_in_use(next)))
				{
					if (!sm_chunk_in_use(next))
					{
						if (next == fmst->top)
						{
							size_t tsiz = fmst->top_size += psiz;

							fmst->top = pchk;
							pchk->head = tsiz | SM_P_IN_USE_BIT;

							if (pchk == fmst->dv)
							{
								fmst->dv = 0;
								fmst->dv_size = UINT64_C(0);
							}

							if (sm_should_trim(fmst, tsiz))
								sm_system_trim(fmst, UINT64_C(0));

							goto LOC_POST_ACTION;
						}
						else if (next == fmst->dv)
						{
							size_t dsiz = fmst->dv_size += psiz;

							fmst->dv = pchk;
							sm_sets_p_inuse_fchunk(pchk, dsiz);

							goto LOC_POST_ACTION;
						}
						else
						{
							size_t nsiz = sm_chunk_size(next);

							psiz += nsiz;

							sm_unlink_chunk(fmst, next, nsiz);
							sm_sets_p_inuse_fchunk(pchk, psiz);

							if (pchk == fmst->dv)
							{
								fmst->dv_size = psiz;

								goto LOC_POST_ACTION;
							}
						}
					}
					else sm_set_free_with_p_in_use(pchk, psiz, next);

					if (sm_is_small(psiz))
					{
						sm_insert_small_chunk(fmst, pchk, psiz);
						sm_check_free_chunk(fmst, pchk);
					}
					else
					{
						sm_ptchunk_t tptr = (sm_ptchunk_t)pchk;

						sm_insert_large_chunk(fmst, tptr, psiz);
						sm_check_free_chunk(fmst, pchk);

						if (--fmst->release_checks == 0)
							sm_release_unused_segments(fmst);
					}

					goto LOC_POST_ACTION;
				}
			}

		LOC_ERROR_ACTION:

			SM_USAGE_ERROR_ACTION(fmst, pchk);

		LOC_POST_ACTION:

			SM_POST_ACTION(fmst);
		}
	}
}


void* sm_space_calloc(sm_space_t space, size_t count, size_t size)
{
	void* pmem;
	size_t sreq = 0;
	sm_state_t mspt = (sm_state_t)space;

	if (!sm_is_magic_ok(mspt))
	{
		SM_USAGE_ERROR_ACTION(mspt, mspt);

		return NULL;
	}

	if (count != 0)
	{
		sreq = count * size;

		if (((count | size) & ~UINT64_C(0xFFFF)) && (sreq / count != size))
			sreq = SM_MAX_SIZE_T;
	}

	pmem = sm_internal_allocate(mspt, sreq);

	if (pmem != NULL && sm_calloc_must_clear(sm_memory_to_chunk(pmem)))
	{
		register uint8_t* mspt = (uint8_t*)pmem;
		while (sreq-- > 0U) *mspt++ = 0;
	}

	return pmem;
}


exported void* callconv sm_space_realloc(sm_space_t space, void* memory, size_t bytes)
{
	void* pmem = NULL;

	if (memory == NULL)
		pmem = sm_space_allocate(space, bytes);
	else if (bytes >= SM_MAX_REQUEST)
		SM_MALLOC_FAILURE_ACTION;
#ifdef SM_REALLOC_ZERO_BYTES_FREES
	else if (bytes == UINT64_C(0))
		sm_space_free(space, memory);
#endif
	else
	{
		size_t bcnt = sm_request_to_size(bytes);
		sm_pchunk_t oldp = sm_memory_to_chunk(memory);

#if !SM_FOOTERS
		sm_state_t msta = (sm_state_t)space;
#else
		sm_state_t msta = sm_get_memory_state_for(oldp);

		if (!sm_is_magic_ok(msta))
		{
			SM_USAGE_ERROR_ACTION(msta, memory);

			return NULL;
		}
#endif

		if (!SM_PRE_ACTION(msta))
		{
			sm_pchunk_t newp = sm_try_reallocate_chunk(msta, oldp, bcnt, 1);

			SM_POST_ACTION(msta);

			if (newp != NULL)
			{
				sm_check_in_use_chunk(msta, newp);
				pmem = sm_chunk_to_memory(newp);
			}
			else
			{
				pmem = sm_space_allocate(msta, bytes);

				if (pmem != NULL)
				{
					size_t ocsz = sm_chunk_size(oldp) - sm_get_overhead_for(oldp);

					sm_memory_copy(pmem, memory, (ocsz < bytes) ? ocsz : bytes);
					sm_space_free(msta, memory);
				}
			}
		}
	}

	return pmem;
}


exported void* callconv sm_space_realloc_in_place(sm_space_t space, void* memory, size_t bytes)
{
	void* pmem = NULL;

	if (memory != NULL)
	{
		if (bytes >= SM_MAX_REQUEST)
		{
			SM_MALLOC_FAILURE_ACTION;
		}
		else
		{
			size_t bcnt = sm_request_to_size(bytes);
			sm_pchunk_t oldp = sm_memory_to_chunk(memory);

#if !SM_FOOTERS
			sm_state_t msta = (sm_state_t)space;
#else
			sm_state_t msta = sm_get_memory_state_for(oldp);

			if (!sm_is_magic_ok(msta))
			{
				SM_USAGE_ERROR_ACTION(msta, memory);

				return NULL;
			}
#endif

			if (!SM_PRE_ACTION(msta))
			{
				sm_pchunk_t newp = sm_try_reallocate_chunk(msta, oldp, bcnt, 0);

				SM_POST_ACTION(msta);

				if (newp == oldp)
				{
					sm_check_in_use_chunk(msta, newp);
					pmem = memory;
				}
			}
		}
	}

	return pmem;
}


exported void* callconv sm_space_memory_align(sm_space_t space, size_t alignment, size_t bytes)
{
	sm_state_t msta = (sm_state_t)space;

	if (!sm_is_magic_ok(msta))
	{
		SM_USAGE_ERROR_ACTION(msta, msta);

		return NULL;
	}

	if (alignment <= SM_MALLOC_ALIGNMENT)
		return sm_space_allocate(space, bytes);

	return sm_internal_memory_align(msta, alignment, bytes);
}


void** sm_space_independent_calloc(sm_space_t space, size_t count, size_t size, void** chunks)
{
	size_t rqsz = size;
	sm_state_t msta = (sm_state_t)space;

	if (!sm_is_magic_ok(msta))
	{
		SM_USAGE_ERROR_ACTION(msta, msta);

		return NULL;
	}

	return sm_independent_allocate(msta, count, &rqsz, 3, chunks);
}


void** sm_space_independent_co_malloc(sm_space_t space, size_t count, size_t* sizes, void** chunks)
{
	sm_state_t msta = (sm_state_t)space;

	if (!sm_is_magic_ok(msta))
	{
		SM_USAGE_ERROR_ACTION(msta, msta);

		return NULL;
	}

	return sm_independent_allocate(msta, count, sizes, 0, chunks);
}


size_t sm_space_bulk_free(sm_space_t space, void** array, size_t count)
{
	return sm_internal_bulk_free((sm_state_t)space, array, count);
}


#if SM_MALLOC_INSPECT_ALL
exported void callconv sm_space_inspect_all(sm_space_t space, void(*visitor)(void* start, void* end, size_t bytes, void* argument), void* argument)
{
	sm_state_t msta = (sm_state_t)space;

	if (sm_is_magic_ok(msta))
	{
		if (!SM_PRE_ACTION(msta))
		{
			sm_internal_inspect_all(msta, visitor, argument);

			SM_POST_ACTION(msta);
		}
	}
	else
	{
		SM_USAGE_ERROR_ACTION(msta, msta);
	}
}
#endif


exported uint8_t callconv sm_space_trim(sm_space_t space, size_t padding)
{
	uint8_t trim = 0;
	sm_state_t msta = (sm_state_t)space;

	if (sm_is_magic_ok(msta))
	{
		if (!SM_PRE_ACTION(msta))
		{
			trim = sm_system_trim(msta, padding);

			SM_POST_ACTION(msta);
		}
	}
	else
	{
		SM_USAGE_ERROR_ACTION(msta, msta);
	}

	return trim;
}


#if !SM_NO_MALLOC_STATS
exported sm_allocation_stats_t callconv sm_space_allocation_statistics(sm_space_t space)
{
	sm_allocation_stats_t stat = { UINT64_C(0), UINT64_C(0), UINT64_C(0) };

	sm_state_t msta = (sm_state_t)space;

	if (sm_is_magic_ok(msta))
		stat = sm_internal_malloc_stats(msta);
	else
	{
		SM_USAGE_ERROR_ACTION(msta, msta);
	}

	return stat;
}
#endif


exported size_t callconv sm_space_footprint(sm_space_t space)
{
	size_t resv = UINT64_C(0);
	sm_state_t msta = (sm_state_t)space;

	if (sm_is_magic_ok(msta))
		resv = msta->foot_print;
	else
	{
		SM_USAGE_ERROR_ACTION(msta, msta);
	}

	return resv;
}


exported size_t callconv sm_space_maximum_footprint(sm_space_t space)
{
	size_t resv = UINT64_C(0);
	sm_state_t msta = (sm_state_t)space;

	if (sm_is_magic_ok(msta))
		resv = msta->max_foot_print;
	else
	{
		SM_USAGE_ERROR_ACTION(msta, msta);
	}

	return resv;
}


exported size_t callconv sm_space_footprint_limit(sm_space_t space)
{
	size_t resv = UINT64_C(0);
	sm_state_t msta = (sm_state_t)space;

	if (sm_is_magic_ok(msta))
	{
		size_t mafl = msta->foot_print_limit;
		resv = (mafl == UINT64_C(0)) ? SM_MAX_SIZE_T : mafl;
	}
	else
	{
		SM_USAGE_ERROR_ACTION(msta, msta);
	}

	return resv;
}


exported size_t callconv sm_space_set_footprint_limit(sm_space_t space, size_t bytes)
{
	size_t resv = UINT64_C(0);
	sm_state_t msta = (sm_state_t)space;

	if (sm_is_magic_ok(msta))
	{
		if (bytes == UINT64_C(0))
			resv = sm_granularity_align(1);

		if (bytes == SM_MAX_SIZE_T)
			resv = UINT64_C(0);
		else resv = sm_granularity_align(bytes);

		msta->foot_print_limit = resv;
	}
	else
	{
		SM_USAGE_ERROR_ACTION(msta, msta);
	}

	return resv;
}


#if !SM_NO_ALLOCATION_INFO
exported struct sm_memory_info_t callconv sm_space_memory_info(sm_space_t space)
{
	sm_state_t msta = (sm_state_t)space;

	if (!sm_is_magic_ok(msta))
	{
		SM_USAGE_ERROR_ACTION(msta, msta);
	}

	return sm_internal_memory_info(msta);
}
#endif


exported size_t callconv sm_space_usable_size(const void* memory)
{
	if (memory != NULL)
	{
		sm_pchunk_t pchk = sm_memory_to_chunk(memory);

		if (sm_is_in_use(pchk))
			return (sm_chunk_size(pchk) - sm_get_overhead_for(pchk));
	}

	return 0;
}


exported int callconv sm_space_options(int param, int value)
{
	return sm_change_param(param, value);
}


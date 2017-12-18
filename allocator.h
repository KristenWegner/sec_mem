// allocator.h


#ifndef INCLUDE_ALLOCATOR_H
#define INCLUDE_ALLOCATOR_H 1


#include <stddef.h>
#include <stdint.h>

#include "config.h"


#define extern

#define SM_MSPACES 1
#define SM_ONLY_MSPACES 1
#define SM_USE_LOCKS 1
#define SM_FOOTERS 1
#define SM_HAVE_MMAP 1
#define SM_PROCEED_ON_ERROR 1
#define SM_MALLOC_INSPECT_ALL 1
#define SM_ALLOCATOR_EXPORT

#undef SM_INSECURE
#undef SM_ABORT
#undef SM_ABORT_ON_ASSERT_FAILURE
#undef SM_LACKS_STDLIB_H
#undef SM_LACKS_STRING_H
#undef SM_NO_ALLOC_STATS

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


#define SM_ONLY_MSPACES 1
#define SM_MSPACES 1


// Tuning options. This malloc supports the following options.
#define SM_M_TRIM_THRESHOLD (-1)
#define SM_M_GRANULARITY    (-2)
#define SM_M_MMAP_THRESHOLD (-3)


#if !SM_NO_ALLOCATION_INFO
#ifndef SM_STRUCT_MEM_INFO_DECLARED
#define SM_STRUCT_MEM_INFO_DECLARED 1
typedef struct sm_memory_info_t
{
	uint64_t arena; // Non-mmapped space allocated from system.
	uint64_t count_free_chunks; // Number of free chunks.
	uint64_t small_blocks; // Always 0.
	uint64_t h_blocks; // Always 0.
	uint64_t memory_mapped_space; // Space in mmapped regions.
	uint64_t max_total_allocated_space; // Maximum total allocated space.
	uint64_t fsm_blocks; // Always 0.
	uint64_t total_allocated_space; // Total allocated space.
	uint64_t total_free_space; // Total free space.
	uint64_t keep_cost; // Releasable (via malloc_trim) space.
}
sm_memory_info_t;
#endif
#endif


#if !SM_NO_MALLOC_STATS
// Allocation statistics.
typedef struct sm_allocation_stats_s
{
	size_t maximum_system_bytes;
	size_t system_bytes;
	size_t in_use_bytes;
}
sm_allocation_stats_t;
#endif


typedef struct sm_context_s
{
#if defined(SM_OS_WINDOWS)
	void* (__stdcall *pvalloc)(void*, size_t, unsigned long, unsigned long);
	size_t (__stdcall *pvquery)(const void*, void*, size_t);
	int (__stdcall *pvfree)(void*, size_t, unsigned long);
	long (*pixchng)(long volatile*, long);
	long (*picxchng)(long volatile*, long, long);
#else

#endif
};

// Represents an independent region of memory space.
typedef void* sm_space_t;


// Creates and returns a new memory space with the given initial capacity, or, if 0, the default granularity size. 
// If argument locked is non-zero, the space uses a separate lock to control access. The capacity of the space will 
// grow dynamically as needed to service allocation requests.
exported sm_space_t callconv sm_create_space(size_t capacity, uint8_t locked);


// Destroys the given memory space, and attempts to return all of its memory back to the system, returning the total 
// number of bytes freed. After destruction, the results of access to all memory used by the space become undefined.
exported size_t callconv sm_destroy_space(sm_space_t space);


// Controls whether requests for large chunks are allocated in their own untracked memory-mapped regions, 
// separate from others in the memory space.
exported uint8_t callconv sm_space_track_large_chunks(sm_space_t space, uint8_t enable);


#if !SM_NO_MEM_INFO
// Reports properties of the given memory space.
exported struct sm_memory_info_t callconv sm_space_memory_info(sm_space_t space);
#endif


// Sets memory space options like mallopt.
exported int callconv sm_space_options(int, int);


// The following operate identically to their malloc counterparts but operate only for the given 
// memory space.


exported void* callconv sm_space_allocate(sm_space_t space, size_t bytes);
exported void callconv sm_space_free(sm_space_t space, void* memory);
exported void* callconv sm_space_realloc(sm_space_t space, void* memory, size_t size);
exported void* callconv sm_space_realloc_in_place(sm_space_t space, void* memory, size_t size);
exported void* callconv sm_space_memory_align(sm_space_t space, size_t alignment, size_t bytes);


/*

// Uses the memory supplied as the initial base of a new sm_space_t. Part (less than 128 * sizeof(size_t) bytes) of this
// space is used for book-keeping, so the capacity must be at least this large. (Otherwise 0 is returned.)
sm_space_t sm_create_space_with_base(void* base, size_t capacity, uint8_t locked);

void* sm_space_calloc(sm_space_t space, size_t count, size_t size);
void** sm_space_independent_calloc(sm_space_t space, size_t count, size_t elem_size, void** chunks);
void** sm_space_independent_co_malloc(sm_space_t space, size_t count, size_t* sizes, void** chunks);
size_t sm_space_bulk_free(sm_space_t space, void** array, size_t count);

*/


exported size_t callconv sm_space_usable_size(const void* memory);
exported uint8_t callconv sm_space_trim(sm_space_t space, size_t padding);
exported size_t callconv sm_space_footprint(sm_space_t space);
exported size_t callconv sm_space_maximum_footprint(sm_space_t space);
exported size_t callconv sm_space_footprint_limit(sm_space_t space);
exported size_t callconv sm_space_set_footprint_limit(sm_space_t space, size_t bytes);


#if SM_MALLOC_INSPECT_ALL
exported void callconv sm_space_inspect_all(sm_space_t space, void (*visitor)(void *, void *, size_t, void*), void* argument);
#endif


#if !SM_NO_MALLOC_STATS
exported sm_allocation_stats_t callconv sm_space_allocation_statistics(sm_space_t space);
#endif


#endif // INCLUDE_ALLOCATOR_H


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
#define SM_ALLOCATOR_EXPORT

#undef SM_INSECURE
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
#else
#undef SM_LACKS_UNISTD_H
#define SM_USE_DEV_RANDOM 1
#undef SM_LACKS_SYS_PARAM_H
#undef SM_LACKS_SYS_MMAN_H
#undef SM_LACKS_SCHED_H
#endif

#ifndef SM_ONLY_MSPACES
#define SM_ONLY_MSPACES 0
#elif SM_ONLY_MSPACES != 0
#define SM_ONLY_MSPACES 1
#endif


#ifndef SM_NO_MEM_INFO
#define SM_NO_MEM_INFO 0
#endif


#ifndef SM_MSPACES
#if SM_ONLY_MSPACES
#define SM_MSPACES 1
#else
#define SM_MSPACES 0
#endif
#endif


// Returns the number of bytes you can actually use in an allocated chunk, which may be more than you requested 
// (although often not) due to alignment and minimum size constraints.
size_t sm_dl_malloc_usable_size(const void*);


#if SM_MSPACES


// Represents an independent region of memory space.
typedef void* sm_mspace_t;


// Creates and returns a new memory space with the given initial capacity, or, if 0, the default granularity size. 
// If argument locked is non-zero, the space uses a separate lock to control access. The capacity of the space will 
// grow dynamically as needed to service allocation requests.
sm_mspace_t sm_create_mspace(size_t capacity, int locked);

// Destroys the given memory space, and attempts to return all of its memory back to the system, returning the total 
// number of bytes freed. After destruction, the results of access to all memory used by the space become undefined.
size_t sm_destroy_mspace(sm_mspace_t space);

// Uses the memory supplied as the initial base of a new sm_mspace_t. Part (less than 128 * sizeof(size_t) bytes) of this
// space is used for bookkeeping, so the capacity must be at least this large. (Otherwise 0 is returned.)
sm_mspace_t sm_create_mspace_with_base(void* base, size_t capacity, int locked);

// Controls whether requests for large chunks are allocated in their own untracked memory-mapped regions, 
// separate from others in the memory space.
int sm_mspace_track_large_chunks(sm_mspace_t space, int enable);

#if !SM_NO_MEM_INFO
// Reports properties of the given memory space.
struct sm_mem_info_t sm_mspace_mem_info(sm_mspace_t space);
#endif

// Sets memory space options like mallopt.
int sm_mspace_options(int, int);

// The following operate identically to their malloc counterparts but operate only for the given 
// memory space.

void* sm_mspace_malloc(sm_mspace_t space, size_t bytes);
void sm_mspace_free(sm_mspace_t space, void* memory);
void* sm_mspace_calloc(sm_mspace_t space, size_t count, size_t size);
void* sm_mspace_realloc(sm_mspace_t space, void* memory, size_t size);
void* sm_mspace_realloc_in_place(sm_mspace_t space, void* memory, size_t size);
void* sm_mspace_mem_align(sm_mspace_t space, size_t alignment, size_t bytes);
void** sm_mspace_independent_calloc(sm_mspace_t space, size_t count, size_t elem_size, void** chunks);
void** sm_mspace_independent_co_malloc(sm_mspace_t space, size_t count, size_t* sizes, void** chunks);
size_t sm_mspace_bulk_free(sm_mspace_t space, void** array, size_t count);
size_t sm_mspace_usable_size(const void* memory);
void sm_mspace_malloc_stats(sm_mspace_t space);
int sm_mspace_trim(sm_mspace_t space, size_t padding);
size_t sm_mspace_footprint(sm_mspace_t space);
size_t sm_mspace_max_footprint(sm_mspace_t space);
size_t sm_mspace_footprint_limit(sm_mspace_t space);
size_t sm_mspace_set_footprint_limit(sm_mspace_t space, size_t bytes);
void sm_mspace_inspect_all(sm_mspace_t space,  void (*visitor)(void *, void *, size_t, void*), void* argument);


#endif


#endif // INCLUDE_ALLOCATOR_H


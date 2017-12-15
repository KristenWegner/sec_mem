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


/*
  Returns the number of bytes you can actually use in
  an allocated chunk, which may be more than you requested (although
  often not) due to alignment and minimum size constraints.
  You can use this many bytes without worrying about
  overwriting other allocated objects. This is not a particularly great
  programming practice. malloc_usable_size can be more useful in
  debugging and assertions, for example:

  p = malloc(n);
  assert(malloc_usable_size(p) >= 256);
*/
size_t dlmalloc_usable_size(const void*);


#if SM_MSPACES


// This is an opaque type representing an independent region of space that supports mspace_malloc, etc.
typedef void* sm_mspace_t;


/*
  Creates and returns a new independent space with the
  given initial capacity, or, if 0, the default granularity size.  It
  returns null if there is no system memory available to create the
  space.  If argument locked is non-zero, the space uses a separate
  lock to control access. The capacity of the space will grow
  dynamically as needed to service mspace_malloc requests.  You can
  control the sizes of incremental increases of this space by
  compiling with a different SM_DEFAULT_GRANULARITY or dynamically
  setting with mallopt(SM_MALLOPT_GRANULARITY, value).
*/
sm_mspace_t create_mspace(size_t capacity, int locked);


/*
  destroy_mspace destroys the given space, and attempts to return all
  of its memory back to the system, returning the total number of
  bytes freed. After destruction, the results of access to all memory
  used by the space become undefined.
*/
size_t destroy_mspace(sm_mspace_t msp);


/*
  create_mspace_with_base uses the memory supplied as the initial base
  of a new sm_mspace_t. Part (less than 128*sizeof(size_t) bytes) of this
  space is used for bookkeeping, so the capacity must be at least this
  large. (Otherwise 0 is returned.) When this initial space is
  exhausted, additional memory will be obtained from the system.
  Destroying this space will deallocate all additionally allocated
  space (if possible) but not the initial base.
*/
sm_mspace_t create_mspace_with_base(void* base, size_t capacity, int locked);


/*
  mspace_track_large_chunks controls whether requests for large chunks
  are allocated in their own untracked mmapped regions, separate from
  others in this sm_mspace_t. By default large chunks are not tracked,
  which reduces fragmentation. However, such chunks are not
  necessarily released to the system upon destroy_mspace.  Enabling
  tracking by setting to true may increase fragmentation, but avoids
  leakage when relying on destroy_mspace to release all memory
  allocated using this space.  The function returns the previous
  setting.
*/
int mspace_track_large_chunks(sm_mspace_t msp, int enable);


#if !SM_NO_MEM_INFO
// This behaves as sm_mem_info_t, but reports properties of the given space.
struct sm_mem_info_t mspace_mem_info(sm_mspace_t msp);
#endif


// An alias for mallopt.
int mspace_mallopt(int, int);


// The following operate identically to their malloc counterparts but operate only for the given 
// sm_mspace_t argument.


void* mspace_malloc(sm_mspace_t msp, size_t bytes);
void mspace_free(sm_mspace_t msp, void* mem);
void* mspace_calloc(sm_mspace_t msp, size_t n_elements, size_t elem_size);
void* mspace_realloc(sm_mspace_t msp, void* mem, size_t newsize);
void* mspace_realloc_in_place(sm_mspace_t msp, void* mem, size_t newsize);
void* mspace_memalign(sm_mspace_t msp, size_t alignment, size_t bytes);
void** mspace_independent_calloc(sm_mspace_t msp, size_t n_elements,size_t elem_size, void* chunks[]);
void** mspace_independent_comalloc(sm_mspace_t msp, size_t n_elements, size_t sizes[], void* chunks[]);
size_t mspace_bulk_free(sm_mspace_t msp, void**, size_t n_elements);
size_t mspace_usable_size(const void* mem);
void mspace_malloc_stats(sm_mspace_t msp);
int mspace_trim(sm_mspace_t msp, size_t pad);
size_t mspace_footprint(sm_mspace_t msp);
size_t mspace_max_footprint(sm_mspace_t msp);
size_t mspace_footprint_limit(sm_mspace_t msp);
size_t mspace_set_footprint_limit(sm_mspace_t msp, size_t bytes);
void mspace_inspect_all(sm_mspace_t msp,  void(*handler)(void *, void *, size_t, void*), void* arg);


#endif


#endif // INCLUDE_ALLOCATOR_H


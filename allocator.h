// allocator.h


#include <stddef.h>
#include <stdint.h>

#include "config.h"


#ifndef INCLUDE_ALLOCATOR_H
#define INCLUDE_ALLOCATOR_H 1


// Tuning options.
#define SM_M_TRIM_THRESHOLD (-1)
#define SM_M_GRANULARITY    (-2)
#define SM_M_MMAP_THRESHOLD (-3)


// Simple allocation statistics.
typedef struct sm_allocation_stats_s
{
	size_t maximum_system_bytes; // Maximum possible system bytes.
	size_t system_bytes; // Current system bytes.
	size_t in_use_bytes; // Total bytes in use.
}
sm_allocation_stats_t;


// Detailed memory usage information.
typedef struct sm_allocation_info_s
{
	uint64_t arena; // Non-memory-mapped space allocated from the system.
	uint64_t count_free_chunks; // Number of free chunks.
	uint64_t small_blocks; // Always 0.
	uint64_t h_blocks; // Always 0.
	uint64_t memory_mapped_space; // Space in mmapped regions.
	uint64_t max_total_allocated_space; // Maximum total allocated space.
	uint64_t fsm_blocks; // Always 0.
	uint64_t total_allocated_space; // Total allocated space.
	uint64_t total_free_space; // Total free space.
	uint64_t keep_cost; // Releasable space (via trim).
}
sm_allocation_info_t;


// Pointer to a memory context.
typedef void* sm_context_t;


exported sm_context_t callconv sm_allocator_create_context(size_t capacity, uint8_t locked);
exported size_t callconv sm_allocator_destroy_context(sm_context_t context);


exported uint8_t callconv sm_space_track_large_chunks(sm_context_t context, uint8_t enable);
exported struct sm_allocation_info_t callconv sm_space_memory_info(sm_context_t context);
exported int callconv sm_space_options(sm_context_t context, int id, int value);
exported void* callconv sm_space_allocate(sm_context_t context, size_t bytes);
exported void callconv sm_space_free(sm_context_t context, void* memory);
exported void* callconv sm_space_realloc(sm_context_t context, void* memory, size_t size);
exported void* callconv sm_space_realloc_in_place(sm_context_t context, void* memory, size_t size);
exported void* callconv sm_space_memory_align(sm_context_t context, size_t alignment, size_t bytes);
exported size_t callconv sm_space_usable_size(const void* memory);
exported uint8_t callconv sm_space_trim(sm_context_t context, size_t padding);
exported size_t callconv sm_space_footprint(sm_context_t context);
exported size_t callconv sm_space_maximum_footprint(sm_context_t context);
exported size_t callconv sm_space_footprint_limit(sm_context_t context);
exported size_t callconv sm_space_set_footprint_limit(sm_context_t context, size_t bytes);
exported void callconv sm_space_inspect_all(sm_context_t context, void (*visitor)(void* start, void* end, size_t bytes, void* argument), void* argument);
exported sm_allocation_stats_t callconv sm_space_allocation_statistics(sm_context_t context);


#endif // INCLUDE_ALLOCATOR_H


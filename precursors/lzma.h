// lzma.h - LZMA compressor declarations.


#include <stdint.h>


#ifndef INCLUDE_LZMA_H
#define INCLUDE_LZMA_H 1


// Result Codes

#define LZMA_RC_OK (0)
#define LZMA_RC_DATA (1)
#define LZMA_RC_MEMORY (2)
#define LZMA_RC_CHECKSUM (3)
#define LZMA_RC_UNSUPPORTED (4)
#define LZMA_RC_PARAMETER (5)
#define LZMA_RC_INPUT_EOF (6)
#define LZMA_RC_OUTPUT_EOF (7)
#define LZMA_RC_READ (8)
#define LZMA_RC_WRITE (9)
#define LZMA_RC_PROGRESS (10)
#define LZMA_RC_FAILURE (11)
#define LZMA_RC_THREAD (12)
#define LZMA_RC_ARCHIVE (16)
#define LZMA_RC_NO_ARCHIVE (17)

typedef uint8_t lzma_rc_t; // Result type.


// Memory allocator/deallocator.
typedef struct lzma_allocator_s
{
	void* state; // Opaque state.
	void* (*allocate)(void* state, size_t size); // Allocate a block of size count of bytes.
	void (*release)(void* state, void* address); // Free memory block starting at address.
}
lzma_allocator_t, *lzma_allocator_ptr_t;


// Data encoder properties.
typedef struct lzma_encoder_properties_s
{
	uint8_t level;
	uint32_t dictionary_size;
	int32_t lc;
	int32_t lp;
	int32_t pb;
	int32_t algorithm;
	int32_t fb;
	int32_t mode_bt;
	int32_t hash_byte_count;
	uint32_t mc;
	uint8_t write_end_mark;
	uint64_t reduce_size;
}
lzma_encoder_properties_t;


// Data properties.
typedef struct lzma_properties_s
{
	uint32_t lc, lp, pb;
	uint32_t dictionary_size;
}
lzma_properties_t;


// Finish Modes

#define LZMA_FINISH_ANY (0)
#define LZMA_FINISH_END (1)

typedef uint8_t lzma_finish_mode_t; // Finish mode type.


// Status Codes

#define LZMA_STATUS_NOT_SPECIFIED (0)
#define LZMA_STATUS_FINISHED_WITH_MARK (1)
#define LZMA_STATUS_NOT_FINISHED (2)
#define LZMA_STATUS_NEEDS_MORE_INPUT (3)
#define LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK (4)

typedef uint8_t lzma_status_t; // Status code type.


// Methods


// Encode from src to dest.
exported lzma_rc_t callconv lzma_encode(uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t src_len, const lzma_encoder_properties_t* properties, uint8_t* properties_encoded, size_t* properties_size, uint8_t write_end_mark, lzma_allocator_ptr_t allocator);

// Decode from src to dest.
exported lzma_rc_t callconv lzma_decode(uint8_t *dest, size_t* dest_len, const uint8_t* src, size_t* src_len, const uint8_t* properties_data, uint32_t properties_size, lzma_finish_mode_t finish_mode, lzma_status_t* status, lzma_allocator_ptr_t allocator);


#endif // INCLUDE_LZMA_H



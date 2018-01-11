// cmp.c - Basic compression.


#include <stdint.h>
#include <stddef.h>

#include "../config.h"
#include "../sm.h"


inline static sm_error_t lzo_size(const void* src, uint64_t slen, uint64_t* dlen)
{
	uint8_t const *ip = (const uint8_t*)src;
	uint8_t const *const ie = ip + slen;
	uint64_t tl = 0;

	while (ip < ie)
	{
		uint64_t ct = *ip++;

		if (ct < (UINT64_C(1) << 5))
		{
			ct++;

			if (ip + ct > ie)
				return SM_ERR_DATA_CORRUPT;

			tl += ct;
			ip += ct;
		}
		else
		{
			uint64_t ln = (ct >> 5);

			if (ln == 7)
				ln += *ip++;

			ln += 2;

			if (ip >= ie)
				return SM_ERR_DATA_CORRUPT;

			ip++;

			tl += ln;
		}
	}

	*dlen = tl;

	return SM_ERR_NO_ERROR;
}


exported sm_error_t callconv lzo_compress(const void *const src, const uint64_t slen, void *dst, uint64_t *const dlen, void* htab)
{
	const uint8_t** hs;
	uint64_t hv;
	const uint8_t *rf;
	const uint8_t *ip = (const uint8_t*)src;
	const uint8_t *const ie = ip + slen;
	uint8_t *op = (uint8_t*)dst;
	const uint8_t *const oe = (dlen == NULL) ? NULL : op + *dlen;
	int64_t lt;
	uint64_t of;
	uint8_t** ht = htab;

	if (!dlen || !ht) return SM_ERR_INVALID_ARGUMENT;

	if (!src)
	{
		if (slen != 0) return SM_ERR_INVALID_ARGUMENT;
		*dlen = UINT64_C(0);
		return SM_ERR_NO_ERROR;
	}

	if (dst == NULL)
	{
		if (dlen != 0) return SM_ERR_INVALID_ARGUMENT;
		return lzo_size(src, slen, dlen);
	}

	register uint8_t* p = (uint8_t*)ht; // Zero the htab.
	register size_t n = sizeof(uint8_t*) * UINT64_C(0x10000);
	while (n-- > UINT64_C(0)) *p++ = 0;

	lt = 0;
	op++;

	hv = ((ip[0] << 8) | ip[1]);

	while (ip + 2 < ie)
	{
		hv = ((hv << 8) | ip[2]);
		hs = ht + (((hv >> (3 * 8 - 0x10)) - hv) & UINT64_C(0xFFFF));
		rf = *hs;
		*hs = ip;

		if (rf < ip && (of = ip - rf - 1) < 0x2000 && ip + 4 < ie &&  rf > (uint8_t*)src &&  rf[0] == ip[0] &&
			rf[1] == ip[1] && rf[2] == ip[2])
		{
			uint64_t ln = UINT64_C(3);
			const uint64_t xl = ie - ip - 2 > 0x108 ? UINT64_C(0x108) : ie - ip - 2;

			if (op - !lt + 3 + 1 >= oe)
				return SM_ERR_OUT_OF_MEMORY;

			op[-lt - INT64_C(1)] = (uint8_t)(lt - 1);
			op -= !lt;

			while (ln < xl && rf[ln] == ip[ln])
				ln++;

			ln -= UINT64_C(2);

			if (ln < 7)
			{
				*op++ = (uint8_t)((of >> 8) + (ln << 5));
				*op++ = (uint8_t)of;
			}
			else
			{
				*op++ = (uint8_t)((of >> 8) + UINT64_C(0xE0));
				*op++ = (uint8_t)(ln - UINT64_C(7));
				*op++ = (uint8_t)of;
			}

			lt = INT64_C(0), op++;

			ip += ln + UINT64_C(1);

			if (ip + 3 >= ie)
			{
				ip++;
				break;
			}

			hv = (uint64_t)((ip[0] << 8) | ip[1]);
			hv = ((hv << 8) | (uint64_t)ip[2]);
			ht[(((hv >> (3 * 8 - 0x10)) - hv) & UINT64_C(0xFFFF))] = ip;
			ip++;
		}
		else
		{
			if (op >= oe)
				return SM_ERR_OUT_OF_MEMORY;

			lt++;
			*op++ = *ip++;

			if (lt == INT64_C(0x20))
			{
				op[-lt - INT64_C(1)] = (uint8_t)(lt - INT64_C(1));
				lt = INT64_C(0), op++;
			}
		}
	}

	if (op + 3 > oe) return SM_ERR_OUT_OF_MEMORY;

	while (ip < ie)
	{
		lt++;
		*op++ = *ip++;

		if (lt == INT64_C(0x20))
		{
			op[-lt - 1] = (uint8_t)(lt - INT64_C(1));
			lt = INT64_C(0), op++;
		}
	}

	op[-lt - INT64_C(1)] = (uint8_t)(lt - INT64_C(1));
	op -= !lt;

	*dlen = op - (uint8_t*)dst;

	return SM_ERR_NO_ERROR;
}


exported sm_error_t callconv lzo_decompress(const void* src, uint64_t slen, void* dst, uint64_t* dlen)
{
	uint8_t const *ip = (const uint8_t*)src;
	uint8_t const *const ie = ip + slen;
	uint8_t *op = (uint8_t*)dst;
	uint8_t const *const oe = (dlen == UINT64_C(0)) ? 0 : op + *dlen;
	uint64_t rl = UINT64_C(0);
	sm_error_t rc = SM_ERR_NO_ERROR;

	if (!dlen)
		return SM_ERR_INVALID_ARGUMENT;

	if (!src)
	{
		if (slen != UINT64_C(0))
			return SM_ERR_INVALID_ARGUMENT;

		*dlen = UINT64_C(0);

		return SM_ERR_NO_ERROR;
	}

	if (dst == NULL)
	{
		if (dlen != NULL)
			return SM_ERR_INVALID_ARGUMENT;

		return lzo_size(src, slen, dlen);
	}

	do
	{
		uint64_t ct = *ip++;

		if (ct < UINT64_C(0x20))
		{
			ct++;

			if (op + ct > oe)
			{
				--ip;
				goto LOC_GUESS;
			}

			if (ip + ct > ie)
				return SM_ERR_DATA_CORRUPT;

			do *op++ = *ip++;
			while (--ct);
		}
		else
		{
			uint64_t ln = (ct >> 5);
			uint8_t *rf = op - ((ct & UINT64_C(0x1F)) << 8) - 1;

			if (ln == UINT64_C(7)) ln += *ip++;

			ln += 2;

			if (op + ln > oe)
			{
				ip -= (ln >= UINT64_C(9)) ? 2 : 1;
				goto LOC_GUESS;
			}

			if (ip >= ie)
				return SM_ERR_DATA_CORRUPT;

			rf -= *ip++;

			if (rf < (uint8_t*)dst)
				return SM_ERR_DATA_CORRUPT;

			do *op++ = *rf++;
			while (--ln);
		}
	} while (ip < ie);

	*dlen = op - (uint8_t*)dst;

	return SM_ERR_NO_ERROR;

LOC_GUESS:

	rc = lzo_size(ip, slen - (ip - (uint8_t*)src), &rl);

	if (rc >= 0)
		*dlen = rl + (op - (uint8_t*)dst);

	return rc;
}


// LZSS embedded compressor, based on code by Scott Vokes.


#ifndef LZSS_DYNAMIC
#undef LZSS_DYNAMIC
#endif


#if LZSS_DYNAMIC

// Parameters for dynamic operation.

#define LZSS_MALLOC(N) malloc(N)
#define LZSS_FREE(P, N) free(P)

#else

// Parameters for static operation.

#define LZSS_STATIC_INPUT_BUFFER_SIZE (32)
#define LZSS_STATIC_WINDOW_BITS       ( 8)
#define LZSS_STATIC_LOOKAHEAD_BITS    ( 4)

#endif


#undef LZSS_LOOP_DETECT
#define LZSS_INDEXED 1 // Use indexing for faster compression.
#define LZSS_MIN_WINDOW_BITS 4
#define LZSS_MAX_WINDOW_BITS 15
#define LZSS_MIN_LOOKAHEAD_BITS 3
#define LZSS_LITERAL_MARKER 1
#define LZSS_BACKREF_MARKER 0


// Encoder Results

typedef int8_t lzss_encoder_result_t; // Encoder result type.

// Encoder result codes.

#define LZSS_ENCODER_OK          INT8_C( 0) // Data sunk into input buffer.
#define LZSS_ENCODER_EMPTY       INT8_C( 1) // Input exhausted.
#define LZSS_ENCODER_MORE        INT8_C( 2) // Poll again for more output.
#define LZSS_ENCODER_DONE        INT8_C( 3) // Encoding is complete.
#define LZSS_ENCODER_ERROR_NULL  INT8_C(-1) // Null argument.
#define LZSS_ENCODER_ERROR_STATE INT8_C(-2) // API misuse.


#if LZSS_DYNAMIC


#define LZSS_ENCODER_WINDOW_BITS(E) ((E)->window_size)
#define LZSS_ENCODER_LOOKAHEAD_BITS(E) ((E)->look_ahead_size)
#define LZSS_ENCODER_INDEX(E) ((E)->search_index)


struct lzss_index_s
{
	uint16_t size;
	int16_t index[];
};

#else


#define LZSS_ENCODER_WINDOW_BITS(V) (LZSS_STATIC_WINDOW_BITS)
#define LZSS_ENCODER_LOOKAHEAD_BITS(V) (LZSS_STATIC_LOOKAHEAD_BITS)
#define LZSS_ENCODER_INDEX(V) (&(V)->search_index)


struct lzss_index_s
{
	uint16_t size;
	int16_t index[2 << LZSS_STATIC_WINDOW_BITS];
};


#endif


#if LZSS_DYNAMIC

#define LZSS_DECODER_INPUT_BUFFER_SIZE(V) ((V)->input_buffer_size)
#define LZSS_DECODER_WINDOW_BITS(V) ((V)->window_size)
#define LZSS_DECODER_LOOKAHEAD_BITS(V) ((V)->look_ahead_size)

#else

#define LZSS_DECODER_INPUT_BUFFER_SIZE(V) LZSS_STATIC_INPUT_BUFFER_SIZE
#define LZSS_DECODER_WINDOW_BITS(V) (LZSS_STATIC_WINDOW_BITS)
#define LZSS_DECODER_LOOKAHEAD_BITS(V) (LZSS_STATIC_LOOKAHEAD_BITS)

#endif


#define LZSS_MATCH_NOT_FOUND ((uint16_t)-1)
#define LZSS_NO_BITS ((uint16_t)-1)


typedef struct lzss_encoder_s
{
	uint16_t input_size; // Bytes in input buffer.
	uint16_t match_scan_index;
	uint16_t match_length;
	uint16_t match_pos;
	uint16_t outgoing_bits; // Enqueued outgoing bits.
	uint8_t outgoing_bits_count;
	uint8_t flags;
	uint8_t state; // Current machine state.
	uint8_t current_byte; // Current byte of output.
	uint8_t bit_index; // Current bit index.
#if LZSS_DYNAMIC
	uint8_t window_size; // 2^n size of window.
	uint8_t look_ahead_size; // 2^n size of look-ahead.
#if LZSS_INDEXED
	struct lzss_index_s* search_index;
#endif
	uint8_t buffer[]; // Input buffer and sliding window for expansion.
#else
#if LZSS_INDEXED
	struct lzss_index_s search_index;
#endif
	uint8_t buffer[2 << LZSS_ENCODER_WINDOW_BITS(_)]; // Input buffer and sliding window for expansion.
#endif
}
lzss_encoder_t;


typedef int8_t lzss_decoder_result_t;


#define LZSS_DECODER_RESULT_OK			  INT8_C( 0) // Data sunk, ready to poll.
#define LZSS_DECODER_RESULT_FULL	      INT8_C( 1) // Out of space in internal buffer.
#define LZSS_DECODER_RESULT_EMPTY         INT8_C( 2) // Input exhausted.
#define LZSS_DECODER_RESULT_DONE          INT8_C( 3) // Output is done.
#define LZSS_DECODER_RESULT_MORE          INT8_C( 4) // More output or data remains, call again w/fresh output buffer.
#define LZSS_DECODER_RESULT_NULL          INT8_C(-1) // Null argument(s).
#define LZSS_DECODER_RESULT_ERROR_UNKNOWN INT8_C(-2) // Unknown error.


typedef struct lzss_decoder_s
{
	uint16_t input_size; // Bytes in input buffer.
	uint16_t input_index; // Offset to next unprocessed input byte.
	uint16_t output_count; // How many bytes to output.
	uint16_t output_index; // Index for bytes to output.
	uint16_t head_index; // Head of window buffer.
	uint8_t state; // Current machine state.
	uint8_t current_byte; // Current byte of input.
	uint8_t bit_index; // Current bit index.
#if LZSS_DYNAMIC
	uint8_t window_size; // Window buffer bits.
	uint8_t look_ahead_size; // Look-ahead bits.
	uint16_t input_buffer_size; // Input buffer size.
	uint8_t buffers[]; // Input buffer, then expansion window buffer.
#else
	uint8_t buffers[(1 << LZSS_DECODER_WINDOW_BITS(_)) + LZSS_DECODER_INPUT_BUFFER_SIZE(_)]; // Input buffer, then expansion window buffer.
#endif
}
lzss_decoder_t;


typedef uint8_t lzss_encoder_state_t;


#define LZSS_ENCODER_STATE_NOT_FULL        UINT8_C(0) // Input buffer not full enough.
#define LZSS_ENCODER_STATE_FILLED          UINT8_C(1) // Buffer is full.
#define LZSS_ENCODER_STATE_SEARCH          UINT8_C(2) // Searching for patterns.
#define LZSS_ENCODER_STATE_YIELD_TAG_BIT   UINT8_C(3) // Yield tag bit.
#define LZSS_ENCODER_STATE_YIELD_LITERAL   UINT8_C(4) // Emit literal byte.
#define LZSS_ENCODER_STATE_YIELD_BR_INDEX  UINT8_C(5) // Yielding back-ref index.
#define LZSS_ENCODER_STATE_YIELD_BR_LENGTH UINT8_C(6) // Yielding back-ref length.
#define LZSS_ENCODER_STATE_SAVE_BACKLOG    UINT8_C(7) // Copying buffer to backlog.
#define LZSS_ENCODER_STATE_FLUSH_BITS      UINT8_C(8) // Flush bit buffer.
#define LZSS_ENCODER_STATE_DONE            UINT8_C(9) // Done.


// Encoder flags
#define LZSS_ENC_IS_FINISHING_FLAG (1)


typedef struct
{
	uint8_t* buffer; // Output buffer.
	size_t buffer_size; // Buffer size.
	size_t* result_size; // Bytes pushed to buffer, so far.
}
lzss_output_info_t;


// States for the polling state machine.


typedef uint8_t lzss_decoder_state_t;


#define LZSS_DECODER_STATE_TAG_BIT           UINT8_C(0) // Tag bit.
#define LZSS_DECODER_STATE_YIELD_LITERAL     UINT8_C(1) // Ready to yield literal byte.
#define LZSS_DECODER_STATE_BACKREF_INDEX_MSB UINT8_C(2) // Most significant byte of index.
#define LZSS_DECODER_STATE_BACKREF_INDEX_LSB UINT8_C(3) // Least significant byte of index.
#define LZSS_DECODER_STATE_BACKREF_COUNT_MSB UINT8_C(4) // Most significant byte of count.
#define LZSS_DECODER_STATE_BACKREF_COUNT_LSB UINT8_C(5) // Least significant byte of count.
#define LZSS_DECODER_STATE_YIELD_BACKREF     UINT8_C(6) // Ready to yield back-reference.


#define LZSS_BACKREF_COUNT_BITS(V) (LZSS_DECODER_LOOKAHEAD_BITS(V))
#define LZSS_BACKREF_INDEX_BITS(V) (LZSS_DECODER_WINDOW_BITS(V))


#if LZSS_DYNAMIC
// Allocate a new encoder struct and its buffers. Returns null on error.
inline static lzss_encoder_t* lzss_encoder_create(uint8_t window_size, uint8_t look_ahead_size);

// Free an encoder.
inline static void lzss_encoder_destroy(lzss_encoder_t *restrict lzss);
#endif

// Reset an encoder.
inline static void lzss_encoder_reset(lzss_encoder_t *restrict lzss);

// Sink up to size bytes from input into the encoder. Param input_size is set to the number of bytes actually sunk 
// (in case a buffer was filled).
inline static lzss_encoder_result_t lzss_encoder_sink(lzss_encoder_t *restrict lzss, uint8_t *restrict input, size_t size, size_t *restrict input_size);

// Poll for output from the encoder, copying at most out_size bytes into output (setting *result_size to the 
// actual amount copied).
inline static lzss_encoder_result_t lzss_encoder_poll(lzss_encoder_t *restrict lzss, uint8_t *restrict output, size_t out_size, size_t *restrict result_size);

// Notify the encoder that the input stream is finished. If the return value is LZSS_ENCODER_MORE, there is still 
// more output, so call lzss_encoder_poll and repeat.
inline static lzss_encoder_result_t lzss_encoder_finish(lzss_encoder_t *restrict lzss);

#if LZSS_DYNAMIC
// Allocate a decoder with an input buffer of input_buffer_size bytes, an expansion buffer size of 2^window_size, 
// and a look-ahead size of 2^look_ahead_size. (The window buffer and lookahead sizes must match the settings used 
// when the data was compressed.) Returns null on error.
inline static lzss_decoder_t* lzss_decoder_create(uint16_t input_buffer_size, uint8_t expansion_buffer_size, uint8_t look_ahead_size);

// Free a decoder.
inline static void lzss_decoder_destroy(lzss_decoder_t *restrict lzss);
#endif

// Reset a decoder.
inline static void lzss_decoder_reset(lzss_decoder_t *restrict lzss);

// Sink at most size bytes from input into the decoder. Param *input_size is set to indicate how many 
// bytes were actually sunk (in case a buffer was filled).
inline static lzss_decoder_result_t lzss_decoder_sink(lzss_decoder_t *restrict lzss, uint8_t *restrict input, size_t size, size_t *restrict input_size);

// Poll for output from the decoder, copying at most out_size bytes into output (setting *result_size 
// to the actual amount copied).
inline static lzss_decoder_result_t lzss_decoder_poll(lzss_decoder_t *restrict lzss, uint8_t *restrict output, size_t out_size, size_t *restrict result_size);

// Notify the decoder that the input stream is finished. If the return value is LZSS_DECODER_RESULT_MORE, 
// there is still more output, so call lzss_decoder_poll and repeat.
inline static lzss_decoder_result_t lzss_decoder_finish(lzss_decoder_t *restrict lzss);

inline static uint16_t lzss_get_input_offset(lzss_encoder_t *restrict lzss);
inline static uint16_t lzss_get_input_buffer_size(lzss_encoder_t *restrict lzss);
inline static uint16_t lzss_get_lookahead_size(lzss_encoder_t *restrict lzss);
inline static void lzss_add_tag_bit(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi, uint8_t tag);
inline static uint8_t lzss_can_take_byte(lzss_output_info_t *restrict oi);
inline static uint8_t lzss_is_finishing(lzss_encoder_t *restrict lzss);
inline static void lzss_save_backlog(lzss_encoder_t *restrict lzss);

// Push count (maximum of 8) bits to the output buffer, which has room.
inline static void lzss_push_bits(lzss_encoder_t *restrict lzss, uint8_t count, uint8_t bits, lzss_output_info_t *restrict oi);
inline static uint8_t lzss_push_outgoing_bits(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi);
inline static void lzss_push_literal_byte(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi);


#if LZSS_DYNAMIC
inline static lzss_encoder_t* lzss_encoder_create(uint8_t window_size, uint8_t look_ahead_size)
{
	if ((window_size < LZSS_MIN_WINDOW_BITS) || (window_size > LZSS_MAX_WINDOW_BITS) || (look_ahead_size < LZSS_MIN_LOOKAHEAD_BITS) || (look_ahead_size >= window_size))
		return NULL;

	size_t buf_sz = (UINT64_C(2) << window_size);

	lzss_encoder_t* lzss = LZSS_MALLOC(sizeof(lzss_encoder_t) + buf_sz);

	if (lzss == NULL) return NULL;

	lzss->window_size = window_size;
	lzss->look_ahead_size = look_ahead_size;
	lzss_encoder_reset(lzss);

#if LZSS_INDEXED

	size_t index_sz = (buf_sz * sizeof(uint16_t));
	lzss->search_index = LZSS_MALLOC(index_sz + sizeof(struct lzss_index_s));

	if (lzss->search_index == NULL) 
	{
		LZSS_FREE(lzss, sizeof(*lzss) + buf_sz);
		return NULL;
	}

	lzss->search_index->size = (uint16_t)index_sz;

#endif

	return lzss;
}


inline static void lzss_encoder_destroy(lzss_encoder_t *restrict lzss)
{
	size_t buf_sz = (UINT64_C(2) << LZSS_ENCODER_WINDOW_BITS(lzss));

#if LZSS_INDEXED

	size_t index_sz = sizeof(struct lzss_index_s) + lzss->search_index->size;
	LZSS_FREE(lzss->search_index, index_sz);

#endif

	LZSS_FREE(lzss, sizeof(lzss_encoder_t) + buf_sz);
}
#endif


inline static void lzss_encoder_reset(lzss_encoder_t *restrict lzss)
{
	size_t buf_sz = (2 << LZSS_ENCODER_WINDOW_BITS(lzss));

	register uint8_t* p = (uint8_t*)lzss->buffer;
	register size_t n = buf_sz;
	while (n-- > UINT64_C(0)) *p++ = UINT8_C(0);

	lzss->input_size = UINT16_C(0);
	lzss->state = LZSS_ENCODER_STATE_NOT_FULL;
	lzss->match_scan_index = UINT16_C(0);
	lzss->flags = UINT8_C(0);
	lzss->bit_index = UINT8_C(0x80);
	lzss->current_byte = UINT8_C(0);
	lzss->match_length = UINT16_C(0);
	lzss->outgoing_bits = UINT16_C(0);
	lzss->outgoing_bits_count = UINT8_C(0);
#ifdef LZSS_LOOP_DETECT
	lzss->loop_detect = (uint32_t)-1;
#endif
}


inline static void* lzss_memcpy(void *restrict p, const void *restrict q, size_t n)
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


inline static lzss_encoder_result_t lzss_encoder_sink(lzss_encoder_t *restrict lzss, uint8_t *restrict input, size_t size, size_t *restrict input_size)
{
	if ((lzss == NULL) || (input == NULL) || (input_size == NULL))
		return LZSS_ENCODER_ERROR_NULL;

	if (lzss_is_finishing(lzss)) return LZSS_ENCODER_ERROR_STATE;
	if (lzss->state != LZSS_ENCODER_STATE_NOT_FULL) return LZSS_ENCODER_ERROR_STATE;

	uint16_t write_offset = lzss_get_input_offset(lzss) + lzss->input_size;
	uint16_t ibs = lzss_get_input_buffer_size(lzss);
	uint16_t rem = ibs - lzss->input_size;
	uint16_t cp_sz = (rem < (uint16_t)size) ? rem : (uint16_t)size;

	lzss_memcpy(&lzss->buffer[write_offset], input, cp_sz);

	*input_size = cp_sz;
	lzss->input_size += cp_sz;

	if (cp_sz == rem) lzss->state = LZSS_ENCODER_STATE_FILLED;

	return LZSS_ENCODER_OK;
}


inline static uint16_t lzss_find_longest_match(lzss_encoder_t *restrict lzss, uint16_t start, uint16_t end, const uint16_t maxlen, uint16_t *restrict match_length);
inline static void lzss_do_indexing(lzss_encoder_t *restrict lzss);
inline static lzss_encoder_state_t lzss_st_step_search(lzss_encoder_t *restrict lzss);
inline static lzss_encoder_state_t lzss_st_yield_tag_bit(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi);
inline static lzss_encoder_state_t lzss_st_yield_literal_enc(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi);
inline static lzss_encoder_state_t lzss_st_yield_br_index(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi);
inline static lzss_encoder_state_t lzss_st_yield_br_length(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi);
inline static lzss_encoder_state_t lzss_st_save_backlog(lzss_encoder_t *restrict lzss);
inline static lzss_encoder_state_t lzss_st_flush_bit_buffer(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi);


inline static lzss_encoder_result_t lzss_encoder_poll(lzss_encoder_t *restrict lzss, uint8_t *restrict output, size_t out_size, size_t *restrict result_size)
{
	if ((lzss == NULL) || (output == NULL) || (result_size == NULL))
		return LZSS_ENCODER_ERROR_NULL;
	
	if (out_size == UINT64_C(0))
		return LZSS_ENCODER_ERROR_STATE;
	
	*result_size = UINT64_C(0);

	lzss_output_info_t oi = { output, out_size, result_size };

	while (1) 
	{
		register uint8_t s = lzss->state;

		
		if (s == LZSS_ENCODER_STATE_NOT_FULL)
			return LZSS_ENCODER_EMPTY;
		else if (s == LZSS_ENCODER_STATE_FILLED) { lzss_do_indexing(lzss); lzss->state = LZSS_ENCODER_STATE_SEARCH; }
		else if (s == LZSS_ENCODER_STATE_SEARCH) lzss->state = lzss_st_step_search(lzss);
		else if (s == LZSS_ENCODER_STATE_YIELD_TAG_BIT) lzss->state = lzss_st_yield_tag_bit(lzss, &oi);
		else if (s == LZSS_ENCODER_STATE_YIELD_LITERAL) lzss->state = lzss_st_yield_literal_enc(lzss, &oi);
		else if (s == LZSS_ENCODER_STATE_YIELD_BR_INDEX) lzss->state = lzss_st_yield_br_index(lzss, &oi);
		else if (s == LZSS_ENCODER_STATE_YIELD_BR_LENGTH) lzss->state = lzss_st_yield_br_length(lzss, &oi);
		else if (s == LZSS_ENCODER_STATE_SAVE_BACKLOG) lzss->state = lzss_st_save_backlog(lzss);
		else if (s == LZSS_ENCODER_STATE_FLUSH_BITS) { lzss->state = lzss_st_flush_bit_buffer(lzss, &oi); goto LOC_DONE; }
		else if (s == LZSS_ENCODER_STATE_DONE) { LOC_DONE: return LZSS_ENCODER_EMPTY; }
		else return LZSS_ENCODER_ERROR_STATE;

		if (lzss->state == s)
			if (*result_size == out_size) 
				return LZSS_ENCODER_MORE;
	}
}


inline static lzss_encoder_result_t lzss_encoder_finish(lzss_encoder_t *restrict lzss)
{
	if (!lzss) return LZSS_ENCODER_ERROR_NULL;

	lzss->flags |= LZSS_ENC_IS_FINISHING_FLAG;

	if (lzss->state == LZSS_ENCODER_STATE_NOT_FULL) 
		lzss->state = LZSS_ENCODER_STATE_FILLED;

	return lzss->state == LZSS_ENCODER_STATE_DONE ? LZSS_ENCODER_DONE : LZSS_ENCODER_MORE;
}


inline static lzss_encoder_state_t lzss_st_step_search(lzss_encoder_t *restrict lzss)
{
	uint16_t window_length = lzss_get_input_buffer_size(lzss);
	uint16_t lookahead_sz = lzss_get_lookahead_size(lzss);
	uint16_t msi = lzss->match_scan_index;
	uint8_t fin = lzss_is_finishing(lzss);

	if (msi > lzss->input_size - (fin ? UINT16_C(1) : lookahead_sz))
		return (fin) ? LZSS_ENCODER_STATE_FLUSH_BITS : LZSS_ENCODER_STATE_SAVE_BACKLOG;

	uint16_t input_offset = lzss_get_input_offset(lzss);
	uint16_t end = input_offset + msi;
	uint16_t start = end - window_length;
	uint16_t max_possible = lookahead_sz;

	if (lzss->input_size - msi < lookahead_sz)
		max_possible = lzss->input_size - msi;

	uint16_t match_length = UINT16_C(0);
	uint16_t match_pos = lzss_find_longest_match(lzss, start, end, max_possible, &match_length);

	if (match_pos == LZSS_MATCH_NOT_FOUND) 
	{
		lzss->match_scan_index++;
		lzss->match_length = UINT16_C(0);

		return LZSS_ENCODER_STATE_YIELD_TAG_BIT;
	}
	else 
	{
		lzss->match_pos = match_pos;
		lzss->match_length = match_length;

		return LZSS_ENCODER_STATE_YIELD_TAG_BIT;
	}
}


inline static lzss_encoder_state_t lzss_st_yield_tag_bit(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi)
{
	if (lzss_can_take_byte(oi)) 
	{
		if (lzss->match_length == UINT16_C(0)) 
		{
			lzss_add_tag_bit(lzss, oi, LZSS_LITERAL_MARKER);

			return LZSS_ENCODER_STATE_YIELD_LITERAL;
		}
		else 
		{
			lzss_add_tag_bit(lzss, oi, LZSS_BACKREF_MARKER);

			lzss->outgoing_bits = lzss->match_pos - UINT16_C(1);
			lzss->outgoing_bits_count = LZSS_ENCODER_WINDOW_BITS(lzss);

			return LZSS_ENCODER_STATE_YIELD_BR_INDEX;
		}
	}
	else return LZSS_ENCODER_STATE_YIELD_TAG_BIT;
}


inline static lzss_encoder_state_t lzss_st_yield_literal_enc(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi)
{
	if (lzss_can_take_byte(oi)) 
	{
		lzss_push_literal_byte(lzss, oi);
		return LZSS_ENCODER_STATE_SEARCH;
	}
	else return LZSS_ENCODER_STATE_YIELD_LITERAL;
}


inline static lzss_encoder_state_t lzss_st_yield_br_index(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi)
{
	if (lzss_can_take_byte(oi))
	{
		if (lzss_push_outgoing_bits(lzss, oi))
			return LZSS_ENCODER_STATE_YIELD_BR_INDEX;
		else 
		{
			lzss->outgoing_bits = lzss->match_length - UINT16_C(1);
			lzss->outgoing_bits_count = LZSS_ENCODER_LOOKAHEAD_BITS(lzss);

			return LZSS_ENCODER_STATE_YIELD_BR_LENGTH;
		}
	}
	else return LZSS_ENCODER_STATE_YIELD_BR_INDEX;
}


inline static lzss_encoder_state_t lzss_st_yield_br_length(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi)
{
	if (lzss_can_take_byte(oi))
	{
		if (lzss_push_outgoing_bits(lzss, oi) > UINT8_C(0))
			return LZSS_ENCODER_STATE_YIELD_BR_LENGTH;
		else 
		{
			lzss->match_scan_index += lzss->match_length;
			lzss->match_length = UINT16_C(0);
			return LZSS_ENCODER_STATE_SEARCH;
		}
	}
	else return LZSS_ENCODER_STATE_YIELD_BR_LENGTH;
}


inline static lzss_encoder_state_t lzss_st_save_backlog(lzss_encoder_t *restrict lzss)
{
	lzss_save_backlog(lzss);

	return LZSS_ENCODER_STATE_NOT_FULL;
}


inline static lzss_encoder_state_t lzss_st_flush_bit_buffer(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi)
{
	if (lzss->bit_index == UINT8_C(0x80))
		return LZSS_ENCODER_STATE_DONE;
	else if (lzss_can_take_byte(oi)) 
	{
		oi->buffer[(*oi->result_size)++] = lzss->current_byte;
		return LZSS_ENCODER_STATE_DONE;
	}
	else return LZSS_ENCODER_STATE_FLUSH_BITS;
}


inline static void lzss_add_tag_bit(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi, uint8_t tag)
{
	lzss_push_bits(lzss, UINT8_C(1), tag, oi);
}


inline static uint16_t lzss_get_input_offset(lzss_encoder_t *restrict lzss)
{
	return lzss_get_input_buffer_size(lzss);
}


inline static uint16_t lzss_get_input_buffer_size(lzss_encoder_t *restrict lzss)
{
	return (UINT16_C(1) << LZSS_ENCODER_WINDOW_BITS(lzss));
}


inline static uint16_t lzss_get_lookahead_size(lzss_encoder_t *restrict lzss)
{
	return (UINT16_C(1) << LZSS_ENCODER_LOOKAHEAD_BITS(lzss));
}


inline static void lzss_do_indexing(lzss_encoder_t *restrict lzss)
{
#if LZSS_INDEXED

	struct lzss_index_s* hsi = LZSS_ENCODER_INDEX(lzss);
	int16_t last[256];

	register uint8_t* p = (uint8_t*)last;
	register size_t n = sizeof(last);
	while (n-- > UINT64_C(0)) *p++ = UINT8_C(0xFF);

	uint8_t *const data = lzss->buffer;
	int16_t *const index = hsi->index;
	const uint16_t input_offset = lzss_get_input_offset(lzss);
	const uint16_t end = input_offset + lzss->input_size;

	for (uint16_t i = UINT16_C(0); i < end; ++i) 
	{
		uint8_t v = data[i];
		int16_t lv = last[v];
		index[i] = lv;
		last[v] = i;
	}

#endif
}


inline static uint8_t lzss_is_finishing(lzss_encoder_t *restrict lzss)
{
	return (uint8_t)(lzss->flags & LZSS_ENC_IS_FINISHING_FLAG);
}


inline static uint8_t lzss_can_take_byte(lzss_output_info_t *restrict oi)
{
	return (uint8_t)(*oi->result_size < oi->buffer_size);
}


inline static uint16_t lzss_find_longest_match(lzss_encoder_t *restrict lzss, uint16_t start, uint16_t end, const uint16_t maxlen, uint16_t *restrict match_length)
{
	uint8_t* buffer = lzss->buffer;
	uint16_t match_maxlen = UINT16_C(0);
	uint16_t match_index = LZSS_MATCH_NOT_FOUND;
	uint16_t len = UINT16_C(0);
	uint8_t *const needlepoint = &buffer[end];

#if LZSS_INDEXED

	struct lzss_index_s* hsi = LZSS_ENCODER_INDEX(lzss);
	int16_t pos = hsi->index[end];

	while ((pos - (int16_t)start) >= INT16_C(0))
	{
		uint8_t *const pospoint = &buffer[pos];
		len = UINT16_C(0);

		if (pospoint[match_maxlen] != needlepoint[match_maxlen]) 
		{
			pos = hsi->index[pos];
			continue;
		}

		for (len = UINT16_C(1); len < maxlen; ++len)
			if (pospoint[len] != needlepoint[len]) 
				break;

		if (len > match_maxlen) 
		{
			match_maxlen = len;
			match_index = pos;

			if (len == maxlen) 
				break;
		}

		pos = hsi->index[pos];
	}

#else

	for (int16_t pos = (end - INT16_C(1)); (pos - (int16_t)start) >= INT16_C(0); --pos)
	{
		uint8_t *const pospoint = &buffer[pos];

		if ((pospoint[match_maxlen] == needlepoint[match_maxlen]) && (*pospoint == *needlepoint)) 
		{
			for (len = UINT16_C(1); len < maxlen; ++len)
				if (pospoint[len] != needlepoint[len]) 
					break;
			
			if (len > match_maxlen)
			{
				match_maxlen = len;
				match_index = pos;

				if (len == maxlen)
					break;
			}
		}
	}

#endif

	const size_t break_even_point = (UINT64_C(1) + LZSS_ENCODER_WINDOW_BITS(lzss) + LZSS_ENCODER_LOOKAHEAD_BITS(lzss));

	if (match_maxlen > (break_even_point / UINT64_C(8)))
	{
		*match_length = match_maxlen;
		return end - match_index;
	}

	return LZSS_MATCH_NOT_FOUND;
}


inline static uint8_t lzss_push_outgoing_bits(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi)
{
	uint8_t count = UINT8_C(0);
	uint8_t bits = UINT8_C(0);

	if (lzss->outgoing_bits_count > UINT8_C(8))
	{
		count = UINT8_C(8);
		bits = lzss->outgoing_bits >> (lzss->outgoing_bits_count - UINT8_C(8));
	}
	else 
	{
		count = lzss->outgoing_bits_count;
		bits = (uint8_t)lzss->outgoing_bits;
	}

	if (count > UINT8_C(0))
	{
		lzss_push_bits(lzss, count, bits, oi);
		lzss->outgoing_bits_count -= count;
	}

	return count;
}


inline static void lzss_push_bits(lzss_encoder_t *restrict lzss, uint8_t count, uint8_t bits, lzss_output_info_t *restrict oi)
{
	if (count == 8 && lzss->bit_index == UINT8_C(0x80))
		oi->buffer[(*oi->result_size)++] = bits;
	else 
	{
		for (int32_t i = ((int32_t)count - INT32_C(1)); i >= INT32_C(0); --i)
		{
			if (bits & (UINT8_C(1) << i))
				lzss->current_byte |= lzss->bit_index;
			
			lzss->bit_index >>= 1;

			if (lzss->bit_index == UINT8_C(0))
			{
				lzss->bit_index = UINT8_C(0x80);
				oi->buffer[(*oi->result_size)++] = lzss->current_byte;
				lzss->current_byte = UINT8_C(0);
			}
		}
	}
}


inline static void lzss_push_literal_byte(lzss_encoder_t *restrict lzss, lzss_output_info_t *restrict oi)
{
	uint16_t processed_offset = lzss->match_scan_index - UINT16_C(1);
	uint16_t input_offset = lzss_get_input_offset(lzss) + processed_offset;
	uint8_t c = lzss->buffer[input_offset];
	lzss_push_bits(lzss, UINT8_C(8), c, oi);
}


inline static void* lzss_memmove(void *restrict dest, const void *restrict src, register size_t n)
{
	register uint8_t* d = (uint8_t*)dest;
	register const uint8_t* s = (uint8_t const*)src;

	if (d < s) while (n-- > UINT64_C(0)) *d++ = *s++;
	else 
	{
		d += n, s += n;
		while (n-- > UINT64_C(0)) *--d = *--s;
	}

	return dest;
}


inline static void lzss_save_backlog(lzss_encoder_t *restrict lzss)
{
	size_t input_buf_sz = lzss_get_input_buffer_size(lzss);
	uint16_t msi = lzss->match_scan_index;
	uint16_t rem = (uint16_t)(input_buf_sz - (size_t)msi);
	uint16_t shift_sz = (uint16_t)(input_buf_sz + (size_t)rem);

	lzss_memmove(&lzss->buffer[0], &lzss->buffer[input_buf_sz - rem], shift_sz);

	lzss->match_scan_index = UINT16_C(0);
	lzss->input_size -= (uint16_t)(input_buf_sz - (size_t)rem);
}


inline static uint16_t lzss_get_bits(lzss_decoder_t *restrict lzss, uint8_t count);
inline static void lzss_push_byte(lzss_decoder_t *restrict lzss, lzss_output_info_t *restrict oi, uint8_t byte);


#if LZSS_DYNAMIC


lzss_decoder_t* lzss_decoder_create(uint16_t input_buffer_size, uint8_t window_size, uint8_t look_ahead_size)
{
	if ((window_size < LZSS_MIN_WINDOW_BITS) || (window_size > LZSS_MAX_WINDOW_BITS) || !input_buffer_size || 
		(look_ahead_size < LZSS_MIN_LOOKAHEAD_BITS) || (look_ahead_size >= window_size))
		return NULL;
	
	size_t buffers_sz = (UINT64_C(1) << window_size) + input_buffer_size;
	size_t sz = sizeof(lzss_decoder_t) + buffers_sz;
	lzss_decoder_t* lzss = LZSS_MALLOC(sz);

	if (!lzss) return NULL;

	lzss->input_buffer_size = input_buffer_size;
	lzss->window_size = window_size;
	lzss->look_ahead_size = look_ahead_size;

	lzss_decoder_reset(lzss);

	return lzss;
}


void lzss_decoder_destroy(lzss_decoder_t *restrict lzss)
{
	size_t buffers_sz = (UINT64_C(1) << lzss->window_size) + lzss->input_buffer_size;
	size_t sz = sizeof(lzss_decoder_t) + buffers_sz;

	LZSS_FREE(lzss, sz);
}


#endif


void lzss_decoder_reset(lzss_decoder_t *restrict lzss)
{
	size_t buf_sz = UINT64_C(1) << LZSS_DECODER_WINDOW_BITS(lzss);
	size_t input_sz = LZSS_DECODER_INPUT_BUFFER_SIZE(lzss);

	register uint8_t* p = (uint8_t*)lzss->buffers;
	register size_t n = (buf_sz + input_sz);
	while (n-- > UINT64_C(0)) *p++ = UINT8_C(0);

	lzss->state = LZSS_DECODER_STATE_TAG_BIT;
	lzss->input_size = UINT16_C(0);
	lzss->input_index = UINT16_C(0);
	lzss->bit_index = UINT8_C(0);
	lzss->current_byte = UINT8_C(0);
	lzss->output_count = UINT16_C(0);
	lzss->output_index = UINT16_C(0);
	lzss->head_index = UINT16_C(0);
}


lzss_decoder_result_t lzss_decoder_sink(lzss_decoder_t *restrict lzss, uint8_t *restrict input, size_t size, size_t *restrict input_size)
{
	if (!lzss || !input || !input_size)
		return LZSS_DECODER_RESULT_NULL;

	size_t rem = LZSS_DECODER_INPUT_BUFFER_SIZE(lzss) - lzss->input_size;

	if (rem == UINT64_C(0)) 
	{
		*input_size = UINT64_C(0);

		return LZSS_DECODER_RESULT_FULL;
	}

	size = rem < size ? rem : size;

	lzss_memcpy(&lzss->buffers[lzss->input_size], input, size);

	lzss->input_size += (uint16_t)size;
	*input_size = size;

	return LZSS_DECODER_RESULT_OK;
}


inline static lzss_decoder_state_t lzss_st_tag_bit(lzss_decoder_t *restrict lzss);
inline static lzss_decoder_state_t lzss_st_yield_literal_dec(lzss_decoder_t *restrict lzss, lzss_output_info_t *restrict oi);
inline static lzss_decoder_state_t lzss_st_backref_index_msb(lzss_decoder_t *restrict lzss);
inline static lzss_decoder_state_t lzss_st_backref_index_lsb(lzss_decoder_t *restrict lzss);
inline static lzss_decoder_state_t lzss_st_backref_count_msb(lzss_decoder_t *restrict lzss);
inline static lzss_decoder_state_t lzss_st_backref_count_lsb(lzss_decoder_t *restrict lzss);
inline static lzss_decoder_state_t lzss_st_yield_backref(lzss_decoder_t *restrict lzss, lzss_output_info_t *restrict oi);


inline static lzss_decoder_result_t lzss_decoder_poll(lzss_decoder_t *restrict lzss, uint8_t *restrict output, size_t out_size, size_t *restrict result_size)
{
	if (!lzss || !output || !result_size)
		return LZSS_DECODER_RESULT_NULL;
	
	*result_size = UINT64_C(0);

	lzss_output_info_t oi;
	oi.buffer = output;
	oi.buffer_size = out_size;
	oi.result_size = result_size;

	while (1) 
	{
		register uint8_t s = lzss->state;

		if (s == LZSS_DECODER_STATE_TAG_BIT)
			lzss->state = lzss_st_tag_bit(lzss);
		else if (s == LZSS_DECODER_STATE_YIELD_LITERAL)
			lzss->state = lzss_st_yield_literal_dec(lzss, &oi);
		else if (s == LZSS_DECODER_STATE_BACKREF_INDEX_MSB)
			lzss->state = lzss_st_backref_index_msb(lzss);
		else if (s == LZSS_DECODER_STATE_BACKREF_INDEX_LSB)
			lzss->state = lzss_st_backref_index_lsb(lzss);
		else if (s == LZSS_DECODER_STATE_BACKREF_COUNT_MSB)
			lzss->state = lzss_st_backref_count_msb(lzss);
		else if (s == LZSS_DECODER_STATE_BACKREF_COUNT_LSB)
			lzss->state = lzss_st_backref_count_lsb(lzss);
		else if (s == LZSS_DECODER_STATE_YIELD_BACKREF)
			lzss->state = lzss_st_yield_backref(lzss, &oi);
		else return LZSS_DECODER_RESULT_ERROR_UNKNOWN;

		if (lzss->state == s) 
		{
			if (*result_size == out_size)
				return LZSS_DECODER_RESULT_MORE;

			return LZSS_DECODER_RESULT_EMPTY;
		}
	}
}


inline static lzss_decoder_state_t lzss_st_tag_bit(lzss_decoder_t *restrict lzss)
{
	uint32_t bits = lzss_get_bits(lzss, UINT8_C(1));

	if (bits == LZSS_NO_BITS) 
		return LZSS_DECODER_STATE_TAG_BIT;
	else if (bits) 
		return LZSS_DECODER_STATE_YIELD_LITERAL;
	else if (LZSS_DECODER_WINDOW_BITS(lzss) > 8)
		return LZSS_DECODER_STATE_BACKREF_INDEX_MSB;
	else 
	{
		lzss->output_index = UINT16_C(0);
		return LZSS_DECODER_STATE_BACKREF_INDEX_LSB;
	}
}


inline static lzss_decoder_state_t lzss_st_yield_literal_dec(lzss_decoder_t *restrict lzss, lzss_output_info_t *restrict oi)
{
	if (*oi->result_size < oi->buffer_size)
	{
		uint16_t byte = lzss_get_bits(lzss, UINT8_C(8));

		if (byte == LZSS_NO_BITS)
			return LZSS_DECODER_STATE_YIELD_LITERAL;

		uint8_t* buffer = &lzss->buffers[LZSS_DECODER_INPUT_BUFFER_SIZE(lzss)];
		uint16_t mask = (UINT16_C(1) << LZSS_DECODER_WINDOW_BITS(lzss)) - UINT16_C(1);
		uint8_t c = byte & UINT8_C(0xFF);

		buffer[lzss->head_index++ & mask] = c;
		lzss_push_byte(lzss, oi, c);

		return LZSS_DECODER_STATE_TAG_BIT;
	}
	else return LZSS_DECODER_STATE_YIELD_LITERAL;
}


inline static lzss_decoder_state_t lzss_st_backref_index_msb(lzss_decoder_t *restrict lzss)
{
	uint8_t bit_ct = LZSS_BACKREF_INDEX_BITS(lzss);
	uint16_t bits = lzss_get_bits(lzss, bit_ct - UINT8_C(8));

	if (bits == LZSS_NO_BITS)
		return LZSS_DECODER_STATE_BACKREF_INDEX_MSB;

	lzss->output_index = bits << 8;

	return LZSS_DECODER_STATE_BACKREF_INDEX_LSB;
}


inline static lzss_decoder_state_t lzss_st_backref_index_lsb(lzss_decoder_t *restrict lzss)
{
	uint8_t bit_ct = LZSS_BACKREF_INDEX_BITS(lzss);
	uint16_t bits = lzss_get_bits(lzss, (bit_ct < UINT8_C(8)) ? bit_ct : UINT8_C(8));
	
	if (bits == LZSS_NO_BITS)
		return LZSS_DECODER_STATE_BACKREF_INDEX_LSB;

	lzss->output_index |= bits;
	lzss->output_index++;

	uint8_t br_bit_ct = LZSS_BACKREF_COUNT_BITS(lzss);

	lzss->output_count = UINT16_C(0);

	return (br_bit_ct > UINT8_C(8)) ? LZSS_DECODER_STATE_BACKREF_COUNT_MSB : LZSS_DECODER_STATE_BACKREF_COUNT_LSB;
}


inline static lzss_decoder_state_t lzss_st_backref_count_msb(lzss_decoder_t *restrict lzss)
{
	uint8_t br_bit_ct = LZSS_BACKREF_COUNT_BITS(lzss);
	uint16_t bits = lzss_get_bits(lzss, br_bit_ct - UINT8_C(8));

	if (bits == LZSS_NO_BITS) 
		return LZSS_DECODER_STATE_BACKREF_COUNT_MSB;

	lzss->output_count = (bits << 8);

	return LZSS_DECODER_STATE_BACKREF_COUNT_LSB;
}


inline static lzss_decoder_state_t lzss_st_backref_count_lsb(lzss_decoder_t *restrict lzss)
{
	uint8_t br_bit_ct = LZSS_BACKREF_COUNT_BITS(lzss);
	uint16_t bits = lzss_get_bits(lzss, (br_bit_ct < UINT8_C(8)) ? br_bit_ct : UINT8_C(8));

	if (bits == LZSS_NO_BITS)
		return LZSS_DECODER_STATE_BACKREF_COUNT_LSB;

	lzss->output_count |= bits;
	lzss->output_count++;

	return LZSS_DECODER_STATE_YIELD_BACKREF;
}


inline static lzss_decoder_state_t lzss_st_yield_backref(lzss_decoder_t *restrict lzss, lzss_output_info_t *restrict oi)
{
	size_t count = oi->buffer_size - *oi->result_size;

	if (count > UINT64_C(0))
	{
		size_t i = UINT64_C(0);

		if (lzss->output_count < count) 
			count = lzss->output_count;

		uint8_t* buffer = &lzss->buffers[LZSS_DECODER_INPUT_BUFFER_SIZE(lzss)];
		uint16_t mask = (UINT16_C(1) << LZSS_DECODER_WINDOW_BITS(lzss)) - UINT16_C(1);
		uint16_t neg_offset = lzss->output_index;

		for (i = UINT64_C(0); i < count; ++i)
		{
			uint8_t c = buffer[(lzss->head_index - neg_offset) & mask];

			lzss_push_byte(lzss, oi, c);

			buffer[lzss->head_index & mask] = c;
			lzss->head_index++;
		}

		lzss->output_count -= (uint16_t)count;

		if (lzss->output_count == UINT16_C(0))
			return LZSS_DECODER_STATE_TAG_BIT;
	}

	return LZSS_DECODER_STATE_YIELD_BACKREF;
}


inline static uint16_t lzss_get_bits(lzss_decoder_t *restrict lzss, uint8_t count)
{
	uint16_t accumulator = UINT16_C(0);
	int32_t i = INT32_C(0);

	if (count > UINT8_C(15)) return LZSS_NO_BITS;

	if (lzss->input_size == UINT16_C(0))
		if (lzss->bit_index < (UINT8_C(1) << (count - UINT8_C(1))))
			return LZSS_NO_BITS;

	for (i = INT32_C(0); i < count; ++i)
	{
		if (lzss->bit_index == UINT8_C(0))
		{
			if (lzss->input_size == UINT16_C(0))
				return LZSS_NO_BITS;
			
			lzss->current_byte = lzss->buffers[lzss->input_index++];

			if (lzss->input_index == lzss->input_size)
			{
				lzss->input_index = UINT16_C(0);
				lzss->input_size = UINT16_C(0);
			}

			lzss->bit_index = UINT8_C(0x80);
		}

		accumulator <<= 1;

		if (lzss->current_byte & lzss->bit_index)
			accumulator |= UINT16_C(1);

		lzss->bit_index >>= 1;
	}

	return accumulator;
}


inline static lzss_decoder_result_t lzss_decoder_finish(lzss_decoder_t *restrict lzss)
{
	if (!lzss) return LZSS_DECODER_RESULT_NULL;

	register uint8_t s = lzss->state;

	if (s == LZSS_DECODER_STATE_TAG_BIT)
		return (lzss->input_size == UINT16_C(0)) ? LZSS_DECODER_RESULT_DONE : LZSS_DECODER_RESULT_MORE;

	if (s == LZSS_DECODER_STATE_BACKREF_INDEX_LSB || s == LZSS_DECODER_STATE_BACKREF_INDEX_MSB || s == LZSS_DECODER_STATE_BACKREF_COUNT_LSB || 
		s == LZSS_DECODER_STATE_BACKREF_COUNT_MSB)
		return (lzss->input_size == UINT16_C(0)) ? LZSS_DECODER_RESULT_DONE : LZSS_DECODER_RESULT_MORE;

	if (s == LZSS_DECODER_STATE_YIELD_LITERAL)
		return (lzss->input_size == UINT16_C(0)) ? LZSS_DECODER_RESULT_DONE : LZSS_DECODER_RESULT_MORE;

	return LZSS_DECODER_RESULT_MORE;
}


inline static void lzss_push_byte(lzss_decoder_t *restrict lzss, lzss_output_info_t *restrict oi, uint8_t byte)
{
	oi->buffer[(*oi->result_size)++] = byte;
}


exported sm_error_t callconv lzss_decompress(const void* src, uint64_t slen, void* dst, uint64_t* dlen)
{
	lzss_encoder_t e;
	lzss_encoder_reset(&e);

	register const uint8_t* s = (uint8_t*)src;
	register uint8_t* p = (uint8_t*)dst;
	register size_t n = *dlen;
	while (n-- > UINT64_C(0)) *p++ = UINT8_C(0);

	uint32_t sunk = 0;
	uint32_t polled = 0;
	size_t count = 0;

	while (sunk < slen)
	{
		if (lzss_encoder_sink(&e, &s[sunk], slen - sunk, &count) >= 0)
			sunk += count;

		if (sunk == slen)
		{
			lzss_encoder_finish(&e);

		lzss_encoder_result_t res;

		do 
		{
			res = lzss_encoder_poll(&e, &p[polled], n - polled, &count);

			if (res < 0) return SM_ERR_OPERATION_FAILED;

			polled += count;
		} 
		while (res == LZSS_ENCODER_MORE);

		if(res != LZSS_ENCODER_EMPTY)
			return SM_ERR_OPERATION_FAILED;

		if (polled >= (slen + (slen / 2) + 4))
			return SM_ERR_OPERATION_FAILED;
	}

	if (sunk == slen && lzss_encoder_finish(&e) != LZSS_ENCODER_DONE)
		return SM_ERR_OPERATION_FAILED;

	*dlen = polled;

	return SM_ERR_NO_ERROR;
}


exported sm_error_t callconv lzss_compress(const void *const src, const uint64_t slen, void *dst, uint64_t *const dlen)
{

}



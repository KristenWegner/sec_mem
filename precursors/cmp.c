// cmp.c - Basic compression.


#include <stdint.h>
#include <stddef.h>

#include "../config.h"
#include "../sm.h"


inline static sm_error_t cmp_size(const void* src, uint64_t slen, uint64_t* dlen)
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


exported sm_error_t callconv cmp_compress(const void *const src, const uint64_t slen, void *dst, uint64_t *const dlen, void* htab)
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
		return cmp_size(src, slen, dlen);
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


exported sm_error_t callconv cmp_decompress(const void* src, uint64_t slen, void* dst, uint64_t* dlen)
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

		return cmp_size(src, slen, dlen);
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

	rc = cmp_size(ip, slen - (ip - (uint8_t*)src), &rl);

	if (rc >= 0)
		*dlen = rl + (op - (uint8_t*)dst);

	return rc;
}




// Copyright (c) 2013-2015, Scott Vokes <vokes.s@gmail.com>, All rights reserved.




#ifndef HEATSHRINK_DYNAMIC_ALLOC
#define HEATSHRINK_DYNAMIC_ALLOC 1
#endif
#if HEATSHRINK_DYNAMIC_ALLOC
// Parameters for dynamic operation.
#define HEATSHRINK_MALLOC(SZ) malloc(SZ)
#define HEATSHRINK_FREE(P, SZ) free(P)
#else
// Parameters for static operation.
#define HEATSHRINK_STATIC_INPUT_BUFFER_SIZE 32
#define HEATSHRINK_STATIC_WINDOW_BITS 8
#define HEATSHRINK_STATIC_LOOKAHEAD_BITS 4
#endif
#define HEATSHRINK_USE_INDEX 1 // Use indexing for faster compression.
#define HEATSHRINK_MIN_WINDOW_BITS 4
#define HEATSHRINK_MAX_WINDOW_BITS 15
#define HEATSHRINK_MIN_LOOKAHEAD_BITS 3
#define HEATSHRINK_LITERAL_MARKER 0x01
#define HEATSHRINK_BACKREF_MARKER 0x00


typedef enum
{
	HSER_SINK_OK, // Data sunk into input buffer.
	HSER_SINK_ERROR_NULL = -1, // Null argument.
	HSER_SINK_ERROR_MISUSE = -2, // API misuse.
}
HSE_sink_res;


typedef enum
{
	HSER_POLL_EMPTY, // Input exhausted.
	HSER_POLL_MORE, // Poll again for more output.
	HSER_POLL_ERROR_NULL = -1, // Null argument.
	HSER_POLL_ERROR_MISUSE = -2, // API misuse.
}
HSE_poll_res;


typedef enum
{
	HSER_FINISH_DONE, // Encoding is complete.
	HSER_FINISH_MORE, // More output remaining; use poll.
	HSER_FINISH_ERROR_NULL = -1, // Null argument.
}
HSE_finish_res;


#if HEATSHRINK_DYNAMIC_ALLOC
#define HEATSHRINK_ENCODER_WINDOW_BITS(E) ((E)->window_sz2)
#define HEATSHRINK_ENCODER_LOOKAHEAD_BITS(E) ((E)->lookahead_sz2)
#define HEATSHRINK_ENCODER_INDEX(E) ((E)->search_index)
struct hs_index
{
	uint16_t size;
	int16_t index[];
};
#else
#define HEATSHRINK_ENCODER_WINDOW_BITS(E) (HEATSHRINK_STATIC_WINDOW_BITS)
#define HEATSHRINK_ENCODER_LOOKAHEAD_BITS(E) (HEATSHRINK_STATIC_LOOKAHEAD_BITS)
#define HEATSHRINK_ENCODER_INDEX(E) (&(E)->search_index)
struct hs_index
{
	uint16_t size;
	int16_t index[2 << HEATSHRINK_STATIC_WINDOW_BITS];
};
#endif


typedef struct heatshrink_encoder_s
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
#if HEATSHRINK_DYNAMIC_ALLOC
	uint8_t window_sz2; // 2^n size of window.
	uint8_t lookahead_sz2; // 2^n size of look-ahead.
#if HEATSHRINK_USE_INDEX
	struct hs_index *search_index;
#endif
	uint8_t buffer[]; // Input buffer and sliding window for expansion.
#else
#if HEATSHRINK_USE_INDEX
	struct hs_index search_index;
#endif
	uint8_t buffer[2 << HEATSHRINK_ENCODER_WINDOW_BITS(_)]; // Input buffer and sliding window for expansion.
#endif
}
heatshrink_encoder;


#if HEATSHRINK_DYNAMIC_ALLOC
// Allocate a new encoder struct and its buffers. Returns null on error.
heatshrink_encoder *heatshrink_encoder_alloc(uint8_t window_sz2, uint8_t lookahead_sz2);
// Free an encoder.
void heatshrink_encoder_free(heatshrink_encoder *hse);
#endif


// Reset an encoder.
void heatshrink_encoder_reset(heatshrink_encoder *hse);

// Sink up to size bytes from in_buf into the encoder. Param input_size is set to the number of bytes actually sunk 
// (in case a buffer was filled).
HSE_sink_res heatshrink_encoder_sink(heatshrink_encoder *hse, uint8_t *in_buf, size_t size, size_t *input_size);

// Poll for output from the encoder, copying at most out_buf_size bytes into out_buf (setting *output_size to the 
// actual amount copied).
HSE_poll_res heatshrink_encoder_poll(heatshrink_encoder *hse, uint8_t *out_buf, size_t out_buf_size, size_t *output_size);

// Notify the encoder that the input stream is finished. If the return value is HSER_FINISH_MORE, there is still 
// more output, so call heatshrink_encoder_poll and repeat.
HSE_finish_res heatshrink_encoder_finish(heatshrink_encoder *hse);


typedef enum
{
	HSDR_SINK_OK, // Data sunk, ready to poll.
	HSDR_SINK_FULL, // Out of space in internal buffer.
	HSDR_SINK_ERROR_NULL = -1, // Null argument.
}
HSD_sink_res;


typedef enum
{
	HSDR_POLL_EMPTY, // Input exhausted */
	HSDR_POLL_MORE, // More data remaining, call again w/fresh output buffer.
	HSDR_POLL_ERROR_NULL = -1, // Null arguments.
	HSDR_POLL_ERROR_UNKNOWN = -2,
}
HSD_poll_res;


typedef enum
{
	HSDR_FINISH_DONE, // Output is done.
	HSDR_FINISH_MORE, // More output remains.
	HSDR_FINISH_ERROR_NULL = -1, // Null arguments.
}
HSD_finish_res;


#if HEATSHRINK_DYNAMIC_ALLOC
#define HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(M) ((M)->input_buffer_size)
#define HEATSHRINK_DECODER_WINDOW_BITS(M) ((M)->window_sz2)
#define HEATSHRINK_DECODER_LOOKAHEAD_BITS(M) ((M)->lookahead_sz2)
#else
#define HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(M) HEATSHRINK_STATIC_INPUT_BUFFER_SIZE
#define HEATSHRINK_DECODER_WINDOW_BITS(M) (HEATSHRINK_STATIC_WINDOW_BITS)
#define HEATSHRINK_DECODER_LOOKAHEAD_BITS(M) (HEATSHRINK_STATIC_LOOKAHEAD_BITS)
#endif


typedef struct heatshrink_decoder_s
{
	uint16_t input_size; // Bytes in input buffer.
	uint16_t input_index; // Offset to next unprocessed input byte.
	uint16_t output_count; // How many bytes to output.
	uint16_t output_index; // Index for bytes to output.
	uint16_t head_index; // Head of window buffer.
	uint8_t state; // Current state machine node.
	uint8_t current_byte; // Current byte of input.
	uint8_t bit_index; // Current bit index.
#if HEATSHRINK_DYNAMIC_ALLOC
	uint8_t window_sz2; // Window buffer bits.
	uint8_t lookahead_sz2; // Look-ahead bits.
	uint16_t input_buffer_size; // Input buffer size.
	uint8_t buffers[]; // Input buffer, then expansion window buffer.
#else
	uint8_t buffers[(1 << HEATSHRINK_DECODER_WINDOW_BITS(_)) + HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(_)]; // Input buffer, then expansion window buffer.
#endif
}
heatshrink_decoder;


#if HEATSHRINK_DYNAMIC_ALLOC
// Allocate a decoder with an input buffer of input_buffer_size bytes, an expansion buffer size of 2^window_sz2, 
// and a look-ahead size of 2^lookahead_sz2. (The window buffer and lookahead sizes must match the settings used 
// when the data was compressed.) Returns null on error.
heatshrink_decoder *heatshrink_decoder_alloc(uint16_t input_buffer_size, uint8_t expansion_buffer_sz2, uint8_t lookahead_sz2);
// Free a decoder.
void heatshrink_decoder_free(heatshrink_decoder *hsd);
#endif

// Reset a decoder.
void heatshrink_decoder_reset(heatshrink_decoder *hsd);

// Sink at most size bytes from in_buf into the decoder. Param *input_size is set to indicate how many 
// bytes were actually sunk (in case a buffer was filled).
HSD_sink_res heatshrink_decoder_sink(heatshrink_decoder *hsd, uint8_t *in_buf, size_t size, size_t *input_size);

// Poll for output from the decoder, copying at most out_buf_size bytes into out_buf (setting *output_size 
// to the actual amount copied).
HSD_poll_res heatshrink_decoder_poll(heatshrink_decoder *hsd, uint8_t *out_buf, size_t out_buf_size, size_t *output_size);

// Notify the decoder that the input stream is finished. If the return value is HSDR_FINISH_MORE, 
// there is still more output, so call heatshrink_decoder_poll and repeat.
HSD_finish_res heatshrink_decoder_finish(heatshrink_decoder *hsd);


typedef enum {
	HSES_NOT_FULL,              /* input buffer not full enough */
	HSES_FILLED,                /* buffer is full */
	HSES_SEARCH,                /* searching for patterns */
	HSES_YIELD_TAG_BIT,         /* yield tag bit */
	HSES_YIELD_LITERAL,         /* emit literal byte */
	HSES_YIELD_BR_INDEX,        /* yielding backref index */
	HSES_YIELD_BR_LENGTH,       /* yielding backref length */
	HSES_SAVE_BACKLOG,          /* copying buffer to backlog */
	HSES_FLUSH_BITS,            /* flush bit buffer */
	HSES_DONE,                  /* done */
} HSE_state;

#if HEATSHRINK_DEBUGGING_LOGS
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#define ASSERT(X) assert(X)
static const char *state_names[] = {
	"not_full",
	"filled",
	"search",
	"yield_tag_bit",
	"yield_literal",
	"yield_br_index",
	"yield_br_length",
	"save_backlog",
	"flush_bits",
	"done",
};
#else
#define LOG(...) /* no-op */
#define ASSERT(X) /* no-op */
#endif

// Encoder flags
enum {
	FLAG_IS_FINISHING = 0x01,
};

typedef struct {
	uint8_t *buf;               /* output buffer */
	size_t buf_size;            /* buffer size */
	size_t *output_size;        /* bytes pushed to buffer, so far */
} output_info;

#define MATCH_NOT_FOUND ((uint16_t)-1)

static uint16_t get_input_offset(heatshrink_encoder *hse);
static uint16_t get_input_buffer_size(heatshrink_encoder *hse);
static uint16_t get_lookahead_size(heatshrink_encoder *hse);
static void add_tag_bit(heatshrink_encoder *hse, output_info *oi, uint8_t tag);
static int can_take_byte(output_info *oi);
static int is_finishing(heatshrink_encoder *hse);
static void save_backlog(heatshrink_encoder *hse);

/* Push COUNT (max 8) bits to the output buffer, which has room. */
static void push_bits(heatshrink_encoder *hse, uint8_t count, uint8_t bits,
	output_info *oi);
static uint8_t push_outgoing_bits(heatshrink_encoder *hse, output_info *oi);
static void push_literal_byte(heatshrink_encoder *hse, output_info *oi);

#if HEATSHRINK_DYNAMIC_ALLOC
heatshrink_encoder *heatshrink_encoder_alloc(uint8_t window_sz2,
	uint8_t lookahead_sz2) {
	if ((window_sz2 < HEATSHRINK_MIN_WINDOW_BITS) ||
		(window_sz2 > HEATSHRINK_MAX_WINDOW_BITS) ||
		(lookahead_sz2 < HEATSHRINK_MIN_LOOKAHEAD_BITS) ||
		(lookahead_sz2 >= window_sz2)) {
		return NULL;
	}

	/* Note: 2 * the window size is used because the buffer needs to fit
	* (1 << window_sz2) bytes for the current input, and an additional
	* (1 << window_sz2) bytes for the previous buffer of input, which
	* will be scanned for useful backreferences. */
	size_t buf_sz = (2 << window_sz2);

	heatshrink_encoder *hse = HEATSHRINK_MALLOC(sizeof(*hse) + buf_sz);
	if (hse == NULL) { return NULL; }
	hse->window_sz2 = window_sz2;
	hse->lookahead_sz2 = lookahead_sz2;
	heatshrink_encoder_reset(hse);

#if HEATSHRINK_USE_INDEX
	size_t index_sz = buf_sz * sizeof(uint16_t);
	hse->search_index = HEATSHRINK_MALLOC(index_sz + sizeof(struct hs_index));
	if (hse->search_index == NULL) {
		HEATSHRINK_FREE(hse, sizeof(*hse) + buf_sz);
		return NULL;
	}
	hse->search_index->size = index_sz;
#endif

	LOG("-- allocated encoder with buffer size of %zu (%u byte input size)\n",
		buf_sz, get_input_buffer_size(hse));
	return hse;
}

void heatshrink_encoder_free(heatshrink_encoder *hse) {
	size_t buf_sz = (2 << HEATSHRINK_ENCODER_WINDOW_BITS(hse));
#if HEATSHRINK_USE_INDEX
	size_t index_sz = sizeof(struct hs_index) + hse->search_index->size;
	HEATSHRINK_FREE(hse->search_index, index_sz);
	(void)index_sz;
#endif
	HEATSHRINK_FREE(hse, sizeof(heatshrink_encoder) + buf_sz);
	(void)buf_sz;
}
#endif

void heatshrink_encoder_reset(heatshrink_encoder *hse) {
	size_t buf_sz = (2 << HEATSHRINK_ENCODER_WINDOW_BITS(hse));
	memset(hse->buffer, 0, buf_sz);
	hse->input_size = 0;
	hse->state = HSES_NOT_FULL;
	hse->match_scan_index = 0;
	hse->flags = 0;
	hse->bit_index = 0x80;
	hse->current_byte = 0x00;
	hse->match_length = 0;

	hse->outgoing_bits = 0x0000;
	hse->outgoing_bits_count = 0;

#ifdef LOOP_DETECT
	hse->loop_detect = (uint32_t)-1;
#endif
}

HSE_sink_res heatshrink_encoder_sink(heatshrink_encoder *hse,
	uint8_t *in_buf, size_t size, size_t *input_size) {
	if ((hse == NULL) || (in_buf == NULL) || (input_size == NULL)) {
		return HSER_SINK_ERROR_NULL;
	}

	/* Sinking more content after saying the content is done, tsk tsk */
	if (is_finishing(hse)) { return HSER_SINK_ERROR_MISUSE; }

	/* Sinking more content before processing is done */
	if (hse->state != HSES_NOT_FULL) { return HSER_SINK_ERROR_MISUSE; }

	uint16_t write_offset = get_input_offset(hse) + hse->input_size;
	uint16_t ibs = get_input_buffer_size(hse);
	uint16_t rem = ibs - hse->input_size;
	uint16_t cp_sz = rem < size ? rem : size;

	memcpy(&hse->buffer[write_offset], in_buf, cp_sz);
	*input_size = cp_sz;
	hse->input_size += cp_sz;

	LOG("-- sunk %u bytes (of %zu) into encoder at %d, input buffer now has %u\n",
		cp_sz, size, write_offset, hse->input_size);
	if (cp_sz == rem) {
		LOG("-- internal buffer is now full\n");
		hse->state = HSES_FILLED;
	}

	return HSER_SINK_OK;
}


/***************
* Compression *
***************/

static uint16_t find_longest_match(heatshrink_encoder *hse, uint16_t start,
	uint16_t end, const uint16_t maxlen, uint16_t *match_length);
static void do_indexing(heatshrink_encoder *hse);

static HSE_state st_step_search(heatshrink_encoder *hse);
static HSE_state st_yield_tag_bit(heatshrink_encoder *hse,
	output_info *oi);
static HSE_state st_yield_literal(heatshrink_encoder *hse,
	output_info *oi);
static HSE_state st_yield_br_index(heatshrink_encoder *hse,
	output_info *oi);
static HSE_state st_yield_br_length(heatshrink_encoder *hse,
	output_info *oi);
static HSE_state st_save_backlog(heatshrink_encoder *hse);
static HSE_state st_flush_bit_buffer(heatshrink_encoder *hse,
	output_info *oi);

HSE_poll_res heatshrink_encoder_poll(heatshrink_encoder *hse,
	uint8_t *out_buf, size_t out_buf_size, size_t *output_size) {
	if ((hse == NULL) || (out_buf == NULL) || (output_size == NULL)) {
		return HSER_POLL_ERROR_NULL;
	}
	if (out_buf_size == 0) {
		LOG("-- MISUSE: output buffer size is 0\n");
		return HSER_POLL_ERROR_MISUSE;
	}
	*output_size = 0;

	output_info oi;
	oi.buf = out_buf;
	oi.buf_size = out_buf_size;
	oi.output_size = output_size;

	while (1) {
		LOG("-- polling, state %u (%s), flags 0x%02x\n",
			hse->state, state_names[hse->state], hse->flags);

		uint8_t in_state = hse->state;
		switch (in_state) {
		case HSES_NOT_FULL:
			return HSER_POLL_EMPTY;
		case HSES_FILLED:
			do_indexing(hse);
			hse->state = HSES_SEARCH;
			break;
		case HSES_SEARCH:
			hse->state = st_step_search(hse);
			break;
		case HSES_YIELD_TAG_BIT:
			hse->state = st_yield_tag_bit(hse, &oi);
			break;
		case HSES_YIELD_LITERAL:
			hse->state = st_yield_literal(hse, &oi);
			break;
		case HSES_YIELD_BR_INDEX:
			hse->state = st_yield_br_index(hse, &oi);
			break;
		case HSES_YIELD_BR_LENGTH:
			hse->state = st_yield_br_length(hse, &oi);
			break;
		case HSES_SAVE_BACKLOG:
			hse->state = st_save_backlog(hse);
			break;
		case HSES_FLUSH_BITS:
			hse->state = st_flush_bit_buffer(hse, &oi);
		case HSES_DONE:
			return HSER_POLL_EMPTY;
		default:
			LOG("-- bad state %s\n", state_names[hse->state]);
			return HSER_POLL_ERROR_MISUSE;
		}

		if (hse->state == in_state) {
			/* Check if output buffer is exhausted. */
			if (*output_size == out_buf_size) return HSER_POLL_MORE;
		}
	}
}

HSE_finish_res heatshrink_encoder_finish(heatshrink_encoder *hse) {
	if (hse == NULL) { return HSER_FINISH_ERROR_NULL; }
	LOG("-- setting is_finishing flag\n");
	hse->flags |= FLAG_IS_FINISHING;
	if (hse->state == HSES_NOT_FULL) { hse->state = HSES_FILLED; }
	return hse->state == HSES_DONE ? HSER_FINISH_DONE : HSER_FINISH_MORE;
}

static HSE_state st_step_search(heatshrink_encoder *hse) {
	uint16_t window_length = get_input_buffer_size(hse);
	uint16_t lookahead_sz = get_lookahead_size(hse);
	uint16_t msi = hse->match_scan_index;
	LOG("## step_search, scan @ +%d (%d/%d), input size %d\n",
		msi, hse->input_size + msi, 2 * window_length, hse->input_size);

	bool fin = is_finishing(hse);
	if (msi > hse->input_size - (fin ? 1 : lookahead_sz)) {
		/* Current search buffer is exhausted, copy it into the
		* backlog and await more input. */
		LOG("-- end of search @ %d\n", msi);
		return fin ? HSES_FLUSH_BITS : HSES_SAVE_BACKLOG;
	}

	uint16_t input_offset = get_input_offset(hse);
	uint16_t end = input_offset + msi;
	uint16_t start = end - window_length;

	uint16_t max_possible = lookahead_sz;
	if (hse->input_size - msi < lookahead_sz) {
		max_possible = hse->input_size - msi;
	}

	uint16_t match_length = 0;
	uint16_t match_pos = find_longest_match(hse,
		start, end, max_possible, &match_length);

	if (match_pos == MATCH_NOT_FOUND) {
		LOG("ss Match not found\n");
		hse->match_scan_index++;
		hse->match_length = 0;
		return HSES_YIELD_TAG_BIT;
	}
	else {
		LOG("ss Found match of %d bytes at %d\n", match_length, match_pos);
		hse->match_pos = match_pos;
		hse->match_length = match_length;
		ASSERT(match_pos <= 1 << HEATSHRINK_ENCODER_WINDOW_BITS(hse) /*window_length*/);

		return HSES_YIELD_TAG_BIT;
	}
}

static HSE_state st_yield_tag_bit(heatshrink_encoder *hse,
	output_info *oi) {
	if (can_take_byte(oi)) {
		if (hse->match_length == 0) {
			add_tag_bit(hse, oi, HEATSHRINK_LITERAL_MARKER);
			return HSES_YIELD_LITERAL;
		}
		else {
			add_tag_bit(hse, oi, HEATSHRINK_BACKREF_MARKER);
			hse->outgoing_bits = hse->match_pos - 1;
			hse->outgoing_bits_count = HEATSHRINK_ENCODER_WINDOW_BITS(hse);
			return HSES_YIELD_BR_INDEX;
		}
	}
	else {
		return HSES_YIELD_TAG_BIT; /* output is full, continue */
	}
}


static HSE_state st_yield_literal(heatshrink_encoder *hse,
	output_info *oi) {
	if (can_take_byte(oi)) {
		push_literal_byte(hse, oi);
		return HSES_SEARCH;
	}
	else {
		return HSES_YIELD_LITERAL;
	}
}


static HSE_state st_yield_br_index(heatshrink_encoder *hse,
	output_info *oi) {
	if (can_take_byte(oi)) {
		LOG("-- yielding backref index %u\n", hse->match_pos);
		if (push_outgoing_bits(hse, oi) > 0) {
			return HSES_YIELD_BR_INDEX; /* continue */
		}
		else {
			hse->outgoing_bits = hse->match_length - 1;
			hse->outgoing_bits_count = HEATSHRINK_ENCODER_LOOKAHEAD_BITS(hse);
			return HSES_YIELD_BR_LENGTH; /* done */
		}
	}
	else {
		return HSES_YIELD_BR_INDEX; /* continue */
	}
}


static HSE_state st_yield_br_length(heatshrink_encoder *hse,
	output_info *oi) {
	if (can_take_byte(oi)) {
		LOG("-- yielding backref length %u\n", hse->match_length);
		if (push_outgoing_bits(hse, oi) > 0) {
			return HSES_YIELD_BR_LENGTH;
		}
		else {
			hse->match_scan_index += hse->match_length;
			hse->match_length = 0;
			return HSES_SEARCH;
		}
	}
	else {
		return HSES_YIELD_BR_LENGTH;
	}
}


static HSE_state st_save_backlog(heatshrink_encoder *hse) {
	LOG("-- saving backlog\n");
	save_backlog(hse);
	return HSES_NOT_FULL;
}


static HSE_state st_flush_bit_buffer(heatshrink_encoder *hse,
	output_info *oi) {
	if (hse->bit_index == 0x80) {
		LOG("-- done!\n");
		return HSES_DONE;
	}
	else if (can_take_byte(oi)) {
		LOG("-- flushing remaining byte (bit_index == 0x%02x)\n", hse->bit_index);
		oi->buf[(*oi->output_size)++] = hse->current_byte;
		LOG("-- done!\n");
		return HSES_DONE;
	}
	else {
		return HSES_FLUSH_BITS;
	}
}


static void add_tag_bit(heatshrink_encoder *hse, output_info *oi, uint8_t tag) {
	LOG("-- adding tag bit: %d\n", tag);
	push_bits(hse, 1, tag, oi);
}


static uint16_t get_input_offset(heatshrink_encoder *hse) {
	return get_input_buffer_size(hse);
}


static uint16_t get_input_buffer_size(heatshrink_encoder *hse) {
	return (1 << HEATSHRINK_ENCODER_WINDOW_BITS(hse));
	(void)hse;
}


static uint16_t get_lookahead_size(heatshrink_encoder *hse) {
	return (1 << HEATSHRINK_ENCODER_LOOKAHEAD_BITS(hse));
	(void)hse;
}


static void do_indexing(heatshrink_encoder *hse) {
#if HEATSHRINK_USE_INDEX
	/* Build an index array I that contains flattened linked lists
	* for the previous instances of every byte in the buffer.
	*
	* For example, if buf[200] == 'x', then index[200] will either
	* be an offset i such that buf[i] == 'x', or a negative offset
	* to indicate end-of-list. This significantly speeds up matching,
	* while only using sizeof(uint16_t)*sizeof(buffer) bytes of RAM.
	*
	* Future optimization options:
	* 1. Since any negative value represents end-of-list, the other
	*    15 bits could be used to improve the index dynamically.
	*
	* 2. Likewise, the last lookahead_sz bytes of the index will
	*    not be usable, so temporary data could be stored there to
	*    dynamically improve the index.
	* */
	struct hs_index *hsi = HEATSHRINK_ENCODER_INDEX(hse);
	int16_t last[256];
	memset(last, 0xFF, sizeof(last));

	uint8_t * const data = hse->buffer;
	int16_t * const index = hsi->index;

	const uint16_t input_offset = get_input_offset(hse);
	const uint16_t end = input_offset + hse->input_size;

	for (uint16_t i = 0; i < end; i++) {
		uint8_t v = data[i];
		int16_t lv = last[v];
		index[i] = lv;
		last[v] = i;
	}
#else
	(void)hse;
#endif
}


static int is_finishing(heatshrink_encoder *hse) {
	return hse->flags & FLAG_IS_FINISHING;
}


static int can_take_byte(output_info *oi) {
	return *oi->output_size < oi->buf_size;
}


/* Return the longest match for the bytes at buf[end:end+maxlen] between
* buf[start] and buf[end-1]. If no match is found, return -1. */
static uint16_t find_longest_match(heatshrink_encoder *hse, uint16_t start,
	uint16_t end, const uint16_t maxlen, uint16_t *match_length) {
	LOG("-- scanning for match of buf[%u:%u] between buf[%u:%u] (max %u bytes)\n",
		end, end + maxlen, start, end + maxlen - 1, maxlen);
	uint8_t *buf = hse->buffer;

	uint16_t match_maxlen = 0;
	uint16_t match_index = MATCH_NOT_FOUND;

	uint16_t len = 0;
	uint8_t * const needlepoint = &buf[end];
#if HEATSHRINK_USE_INDEX
	struct hs_index *hsi = HEATSHRINK_ENCODER_INDEX(hse);
	int16_t pos = hsi->index[end];

	while (pos - (int16_t)start >= 0) {
		uint8_t * const pospoint = &buf[pos];
		len = 0;

		/* Only check matches that will potentially beat the current maxlen.
		* This is redundant with the index if match_maxlen is 0, but the
		* added branch overhead to check if it == 0 seems to be worse. */
		if (pospoint[match_maxlen] != needlepoint[match_maxlen]) {
			pos = hsi->index[pos];
			continue;
		}

		for (len = 1; len < maxlen; len++) {
			if (pospoint[len] != needlepoint[len]) break;
		}

		if (len > match_maxlen) {
			match_maxlen = len;
			match_index = pos;
			if (len == maxlen) { break; } /* won't find better */
		}
		pos = hsi->index[pos];
	}
#else    
	for (int16_t pos = end - 1; pos - (int16_t)start >= 0; pos--) {
		uint8_t * const pospoint = &buf[pos];
		if ((pospoint[match_maxlen] == needlepoint[match_maxlen])
			&& (*pospoint == *needlepoint)) {
			for (len = 1; len < maxlen; len++) {
				if (0) {
					LOG("  --> cmp buf[%d] == 0x%02x against %02x (start %u)\n",
						pos + len, pospoint[len], needlepoint[len], start);
				}
				if (pospoint[len] != needlepoint[len]) { break; }
			}
			if (len > match_maxlen) {
				match_maxlen = len;
				match_index = pos;
				if (len == maxlen) { break; } /* don't keep searching */
			}
		}
	}
#endif

	const size_t break_even_point =
		(1 + HEATSHRINK_ENCODER_WINDOW_BITS(hse) +
			HEATSHRINK_ENCODER_LOOKAHEAD_BITS(hse));

	/* Instead of comparing break_even_point against 8*match_maxlen,
	* compare match_maxlen against break_even_point/8 to avoid
	* overflow. Since MIN_WINDOW_BITS and MIN_LOOKAHEAD_BITS are 4 and
	* 3, respectively, break_even_point/8 will always be at least 1. */
	if (match_maxlen > (break_even_point / 8)) {
		LOG("-- best match: %u bytes at -%u\n",
			match_maxlen, end - match_index);
		*match_length = match_maxlen;
		return end - match_index;
	}
	LOG("-- none found\n");
	return MATCH_NOT_FOUND;
}


static uint8_t push_outgoing_bits(heatshrink_encoder *hse, output_info *oi) {
	uint8_t count = 0;
	uint8_t bits = 0;
	if (hse->outgoing_bits_count > 8) {
		count = 8;
		bits = hse->outgoing_bits >> (hse->outgoing_bits_count - 8);
	}
	else {
		count = hse->outgoing_bits_count;
		bits = hse->outgoing_bits;
	}

	if (count > 0) {
		LOG("-- pushing %d outgoing bits: 0x%02x\n", count, bits);
		push_bits(hse, count, bits, oi);
		hse->outgoing_bits_count -= count;
	}
	return count;
}


/* Push COUNT (max 8) bits to the output buffer, which has room.
* Bytes are set from the lowest bits, up. */
static void push_bits(heatshrink_encoder *hse, uint8_t count, uint8_t bits,
	output_info *oi) {
	ASSERT(count <= 8);
	LOG("++ push_bits: %d bits, input of 0x%02x\n", count, bits);

	/* If adding a whole byte and at the start of a new output byte,
	* just push it through whole and skip the bit IO loop. */
	if (count == 8 && hse->bit_index == 0x80) {
		oi->buf[(*oi->output_size)++] = bits;
	}
	else {
		for (int i = count - 1; i >= 0; i--) {
			bool bit = bits & (1 << i);
			if (bit) { hse->current_byte |= hse->bit_index; }
			if (0) {
				LOG("  -- setting bit %d at bit index 0x%02x, byte => 0x%02x\n",
					bit ? 1 : 0, hse->bit_index, hse->current_byte);
			}
			hse->bit_index >>= 1;
			if (hse->bit_index == 0x00) {
				hse->bit_index = 0x80;
				LOG(" > pushing byte 0x%02x\n", hse->current_byte);
				oi->buf[(*oi->output_size)++] = hse->current_byte;
				hse->current_byte = 0x00;
			}
		}
	}
}


static void push_literal_byte(heatshrink_encoder *hse, output_info *oi) {
	uint16_t processed_offset = hse->match_scan_index - 1;
	uint16_t input_offset = get_input_offset(hse) + processed_offset;
	uint8_t c = hse->buffer[input_offset];
	LOG("-- yielded literal byte 0x%02x ('%c') from +%d\n",
		c, isprint(c) ? c : '.', input_offset);
	push_bits(hse, 8, c, oi);
}


static void save_backlog(heatshrink_encoder *hse) {
	size_t input_buf_sz = get_input_buffer_size(hse);

	uint16_t msi = hse->match_scan_index;

	/* Copy processed data to beginning of buffer, so it can be
	* used for future matches. Don't bother checking whether the
	* input is less than the maximum size, because if it isn't,
	* we're done anyway. */
	uint16_t rem = input_buf_sz - msi; // unprocessed bytes
	uint16_t shift_sz = input_buf_sz + rem;

	memmove(&hse->buffer[0],
		&hse->buffer[input_buf_sz - rem],
		shift_sz);

	hse->match_scan_index = 0;
	hse->input_size -= input_buf_sz - rem;
}


/* States for the polling state machine. */
typedef enum {
	HSDS_TAG_BIT,               /* tag bit */
	HSDS_YIELD_LITERAL,         /* ready to yield literal byte */
	HSDS_BACKREF_INDEX_MSB,     /* most significant byte of index */
	HSDS_BACKREF_INDEX_LSB,     /* least significant byte of index */
	HSDS_BACKREF_COUNT_MSB,     /* most significant byte of count */
	HSDS_BACKREF_COUNT_LSB,     /* least significant byte of count */
	HSDS_YIELD_BACKREF,         /* ready to yield back-reference */
} 
HSD_state;


#if HEATSHRINK_DEBUGGING_LOGS
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#define ASSERT(X) assert(X)
static const char *state_names[] = {
	"tag_bit",
	"yield_literal",
	"backref_index_msb",
	"backref_index_lsb",
	"backref_count_msb",
	"backref_count_lsb",
	"yield_backref",
};
#else
#define LOG(...) /* no-op */
#define ASSERT(X) /* no-op */
#endif


#define NO_BITS ((uint16_t)-1)

/* Forward references. */
static uint16_t get_bits(heatshrink_decoder *hsd, uint8_t count);
static void push_byte(heatshrink_decoder *hsd, output_info *oi, uint8_t byte);

#if HEATSHRINK_DYNAMIC_ALLOC
heatshrink_decoder *heatshrink_decoder_alloc(uint16_t input_buffer_size,
	uint8_t window_sz2,
	uint8_t lookahead_sz2) {
	if ((window_sz2 < HEATSHRINK_MIN_WINDOW_BITS) ||
		(window_sz2 > HEATSHRINK_MAX_WINDOW_BITS) ||
		(input_buffer_size == 0) ||
		(lookahead_sz2 < HEATSHRINK_MIN_LOOKAHEAD_BITS) ||
		(lookahead_sz2 >= window_sz2)) {
		return NULL;
	}
	size_t buffers_sz = (1 << window_sz2) + input_buffer_size;
	size_t sz = sizeof(heatshrink_decoder) + buffers_sz;
	heatshrink_decoder *hsd = HEATSHRINK_MALLOC(sz);
	if (hsd == NULL) { return NULL; }
	hsd->input_buffer_size = input_buffer_size;
	hsd->window_sz2 = window_sz2;
	hsd->lookahead_sz2 = lookahead_sz2;
	heatshrink_decoder_reset(hsd);
	LOG("-- allocated decoder with buffer size of %zu (%zu + %u + %u)\n",
		sz, sizeof(heatshrink_decoder), (1 << window_sz2), input_buffer_size);
	return hsd;
}

void heatshrink_decoder_free(heatshrink_decoder *hsd) {
	size_t buffers_sz = (1 << hsd->window_sz2) + hsd->input_buffer_size;
	size_t sz = sizeof(heatshrink_decoder) + buffers_sz;
	HEATSHRINK_FREE(hsd, sz);
	(void)sz;   /* may not be used by free */
}
#endif

void heatshrink_decoder_reset(heatshrink_decoder *hsd) {
	size_t buf_sz = 1 << HEATSHRINK_DECODER_WINDOW_BITS(hsd);
	size_t input_sz = HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(hsd);
	memset(hsd->buffers, 0, buf_sz + input_sz);
	hsd->state = HSDS_TAG_BIT;
	hsd->input_size = 0;
	hsd->input_index = 0;
	hsd->bit_index = 0x00;
	hsd->current_byte = 0x00;
	hsd->output_count = 0;
	hsd->output_index = 0;
	hsd->head_index = 0;
}

/* Copy SIZE bytes into the decoder's input buffer, if it will fit. */
HSD_sink_res heatshrink_decoder_sink(heatshrink_decoder *hsd,
	uint8_t *in_buf, size_t size, size_t *input_size) {
	if ((hsd == NULL) || (in_buf == NULL) || (input_size == NULL)) {
		return HSDR_SINK_ERROR_NULL;
	}

	size_t rem = HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(hsd) - hsd->input_size;
	if (rem == 0) {
		*input_size = 0;
		return HSDR_SINK_FULL;
	}

	size = rem < size ? rem : size;
	LOG("-- sinking %zd bytes\n", size);
	/* copy into input buffer (at head of buffers) */
	memcpy(&hsd->buffers[hsd->input_size], in_buf, size);
	hsd->input_size += size;
	*input_size = size;
	return HSDR_SINK_OK;
}


/*****************
* Decompression *
*****************/

#define BACKREF_COUNT_BITS(HSD) (HEATSHRINK_DECODER_LOOKAHEAD_BITS(HSD))
#define BACKREF_INDEX_BITS(HSD) (HEATSHRINK_DECODER_WINDOW_BITS(HSD))

// States
static HSD_state st_tag_bit(heatshrink_decoder *hsd);
static HSD_state st_yield_literal(heatshrink_decoder *hsd,
	output_info *oi);
static HSD_state st_backref_index_msb(heatshrink_decoder *hsd);
static HSD_state st_backref_index_lsb(heatshrink_decoder *hsd);
static HSD_state st_backref_count_msb(heatshrink_decoder *hsd);
static HSD_state st_backref_count_lsb(heatshrink_decoder *hsd);
static HSD_state st_yield_backref(heatshrink_decoder *hsd,
	output_info *oi);

HSD_poll_res heatshrink_decoder_poll(heatshrink_decoder *hsd,
	uint8_t *out_buf, size_t out_buf_size, size_t *output_size) {
	if ((hsd == NULL) || (out_buf == NULL) || (output_size == NULL)) {
		return HSDR_POLL_ERROR_NULL;
	}
	*output_size = 0;

	output_info oi;
	oi.buf = out_buf;
	oi.buf_size = out_buf_size;
	oi.output_size = output_size;

	while (1) {
		LOG("-- poll, state is %d (%s), input_size %d\n",
			hsd->state, state_names[hsd->state], hsd->input_size);
		uint8_t in_state = hsd->state;
		switch (in_state) {
		case HSDS_TAG_BIT:
			hsd->state = st_tag_bit(hsd);
			break;
		case HSDS_YIELD_LITERAL:
			hsd->state = st_yield_literal(hsd, &oi);
			break;
		case HSDS_BACKREF_INDEX_MSB:
			hsd->state = st_backref_index_msb(hsd);
			break;
		case HSDS_BACKREF_INDEX_LSB:
			hsd->state = st_backref_index_lsb(hsd);
			break;
		case HSDS_BACKREF_COUNT_MSB:
			hsd->state = st_backref_count_msb(hsd);
			break;
		case HSDS_BACKREF_COUNT_LSB:
			hsd->state = st_backref_count_lsb(hsd);
			break;
		case HSDS_YIELD_BACKREF:
			hsd->state = st_yield_backref(hsd, &oi);
			break;
		default:
			return HSDR_POLL_ERROR_UNKNOWN;
		}

		/* If the current state cannot advance, check if input or output
		* buffer are exhausted. */
		if (hsd->state == in_state) {
			if (*output_size == out_buf_size) { return HSDR_POLL_MORE; }
			return HSDR_POLL_EMPTY;
		}
	}
}

static HSD_state st_tag_bit(heatshrink_decoder *hsd) {
	uint32_t bits = get_bits(hsd, 1);  // get tag bit
	if (bits == NO_BITS) {
		return HSDS_TAG_BIT;
	}
	else if (bits) {
		return HSDS_YIELD_LITERAL;
	}
	else if (HEATSHRINK_DECODER_WINDOW_BITS(hsd) > 8) {
		return HSDS_BACKREF_INDEX_MSB;
	}
	else {
		hsd->output_index = 0;
		return HSDS_BACKREF_INDEX_LSB;
	}
}

static HSD_state st_yield_literal(heatshrink_decoder *hsd,
	output_info *oi) {
	/* Emit a repeated section from the window buffer, and add it (again)
	* to the window buffer. (Note that the repetition can include
	* itself.)*/
	if (*oi->output_size < oi->buf_size) {
		uint16_t byte = get_bits(hsd, 8);
		if (byte == NO_BITS) { return HSDS_YIELD_LITERAL; } /* out of input */
		uint8_t *buf = &hsd->buffers[HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(hsd)];
		uint16_t mask = (1 << HEATSHRINK_DECODER_WINDOW_BITS(hsd)) - 1;
		uint8_t c = byte & 0xFF;
		LOG("-- emitting literal byte 0x%02x ('%c')\n", c, isprint(c) ? c : '.');
		buf[hsd->head_index++ & mask] = c;
		push_byte(hsd, oi, c);
		return HSDS_TAG_BIT;
	}
	else {
		return HSDS_YIELD_LITERAL;
	}
}

static HSD_state st_backref_index_msb(heatshrink_decoder *hsd) {
	uint8_t bit_ct = BACKREF_INDEX_BITS(hsd);
	ASSERT(bit_ct > 8);
	uint16_t bits = get_bits(hsd, bit_ct - 8);
	LOG("-- backref index (msb), got 0x%04x (+1)\n", bits);
	if (bits == NO_BITS) { return HSDS_BACKREF_INDEX_MSB; }
	hsd->output_index = bits << 8;
	return HSDS_BACKREF_INDEX_LSB;
}

static HSD_state st_backref_index_lsb(heatshrink_decoder *hsd) {
	uint8_t bit_ct = BACKREF_INDEX_BITS(hsd);
	uint16_t bits = get_bits(hsd, bit_ct < 8 ? bit_ct : 8);
	LOG("-- backref index (lsb), got 0x%04x (+1)\n", bits);
	if (bits == NO_BITS) { return HSDS_BACKREF_INDEX_LSB; }
	hsd->output_index |= bits;
	hsd->output_index++;
	uint8_t br_bit_ct = BACKREF_COUNT_BITS(hsd);
	hsd->output_count = 0;
	return (br_bit_ct > 8) ? HSDS_BACKREF_COUNT_MSB : HSDS_BACKREF_COUNT_LSB;
}

static HSD_state st_backref_count_msb(heatshrink_decoder *hsd) {
	uint8_t br_bit_ct = BACKREF_COUNT_BITS(hsd);
	ASSERT(br_bit_ct > 8);
	uint16_t bits = get_bits(hsd, br_bit_ct - 8);
	LOG("-- backref count (msb), got 0x%04x (+1)\n", bits);
	if (bits == NO_BITS) { return HSDS_BACKREF_COUNT_MSB; }
	hsd->output_count = bits << 8;
	return HSDS_BACKREF_COUNT_LSB;
}

static HSD_state st_backref_count_lsb(heatshrink_decoder *hsd) {
	uint8_t br_bit_ct = BACKREF_COUNT_BITS(hsd);
	uint16_t bits = get_bits(hsd, br_bit_ct < 8 ? br_bit_ct : 8);
	LOG("-- backref count (lsb), got 0x%04x (+1)\n", bits);
	if (bits == NO_BITS) { return HSDS_BACKREF_COUNT_LSB; }
	hsd->output_count |= bits;
	hsd->output_count++;
	return HSDS_YIELD_BACKREF;
}

static HSD_state st_yield_backref(heatshrink_decoder *hsd,
	output_info *oi) {
	size_t count = oi->buf_size - *oi->output_size;
	if (count > 0) {
		size_t i = 0;
		if (hsd->output_count < count) count = hsd->output_count;
		uint8_t *buf = &hsd->buffers[HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(hsd)];
		uint16_t mask = (1 << HEATSHRINK_DECODER_WINDOW_BITS(hsd)) - 1;
		uint16_t neg_offset = hsd->output_index;
		LOG("-- emitting %zu bytes from -%u bytes back\n", count, neg_offset);
		ASSERT(neg_offset <= mask + 1);
		ASSERT(count <= (size_t)(1 << BACKREF_COUNT_BITS(hsd)));

		for (i = 0; i < count; i++) {
			uint8_t c = buf[(hsd->head_index - neg_offset) & mask];
			push_byte(hsd, oi, c);
			buf[hsd->head_index & mask] = c;
			hsd->head_index++;
			LOG("  -- ++ 0x%02x\n", c);
		}
		hsd->output_count -= count;
		if (hsd->output_count == 0) { return HSDS_TAG_BIT; }
	}
	return HSDS_YIELD_BACKREF;
}

/* Get the next COUNT bits from the input buffer, saving incremental progress.
* Returns NO_BITS on end of input, or if more than 15 bits are requested. */
static uint16_t get_bits(heatshrink_decoder *hsd, uint8_t count) {
	uint16_t accumulator = 0;
	int i = 0;
	if (count > 15) { return NO_BITS; }
	LOG("-- popping %u bit(s)\n", count);

	/* If we aren't able to get COUNT bits, suspend immediately, because we
	* don't track how many bits of COUNT we've accumulated before suspend. */
	if (hsd->input_size == 0) {
		if (hsd->bit_index < (1 << (count - 1))) { return NO_BITS; }
	}

	for (i = 0; i < count; i++) {
		if (hsd->bit_index == 0x00) {
			if (hsd->input_size == 0) {
				LOG("  -- out of bits, suspending w/ accumulator of %u (0x%02x)\n",
					accumulator, accumulator);
				return NO_BITS;
			}
			hsd->current_byte = hsd->buffers[hsd->input_index++];
			LOG("  -- pulled byte 0x%02x\n", hsd->current_byte);
			if (hsd->input_index == hsd->input_size) {
				hsd->input_index = 0; /* input is exhausted */
				hsd->input_size = 0;
			}
			hsd->bit_index = 0x80;
		}
		accumulator <<= 1;
		if (hsd->current_byte & hsd->bit_index) {
			accumulator |= 0x01;
			if (0) {
				LOG("  -- got 1, accumulator 0x%04x, bit_index 0x%02x\n",
					accumulator, hsd->bit_index);
			}
		}
		else {
			if (0) {
				LOG("  -- got 0, accumulator 0x%04x, bit_index 0x%02x\n",
					accumulator, hsd->bit_index);
			}
		}
		hsd->bit_index >>= 1;
	}

	if (count > 1) { LOG("  -- accumulated %08x\n", accumulator); }
	return accumulator;
}

HSD_finish_res heatshrink_decoder_finish(heatshrink_decoder *hsd) {
	if (hsd == NULL) { return HSDR_FINISH_ERROR_NULL; }
	switch (hsd->state) {
	case HSDS_TAG_BIT:
		return hsd->input_size == 0 ? HSDR_FINISH_DONE : HSDR_FINISH_MORE;

		/* If we want to finish with no input, but are in these states, it's
		* because the 0-bit padding to the last byte looks like a backref
		* marker bit followed by all 0s for index and count bits. */
	case HSDS_BACKREF_INDEX_LSB:
	case HSDS_BACKREF_INDEX_MSB:
	case HSDS_BACKREF_COUNT_LSB:
	case HSDS_BACKREF_COUNT_MSB:
		return hsd->input_size == 0 ? HSDR_FINISH_DONE : HSDR_FINISH_MORE;

		/* If the output stream is padded with 0xFFs (possibly due to being in
		* flash memory), also explicitly check the input size rather than
		* uselessly returning MORE but yielding 0 bytes when polling. */
	case HSDS_YIELD_LITERAL:
		return hsd->input_size == 0 ? HSDR_FINISH_DONE : HSDR_FINISH_MORE;

	default:
		return HSDR_FINISH_MORE;
	}
}

static void push_byte(heatshrink_decoder *hsd, output_info *oi, uint8_t byte) {
	LOG(" -- pushing byte: 0x%02x ('%c')\n", byte, isprint(byte) ? byte : '.');
	oi->buf[(*oi->output_size)++] = byte;
	(void)hsd;
}


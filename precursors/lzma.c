// lzma.c - LZMA2 compression support.


#include <stdint.h>

#include "../config.h"


typedef enum lzma_rc_e
{
	LZMA_RC_OK = 0,
	LZMA_RC_DATA = 1,
	LZMA_RC_MEMORY = 2,
	LZMA_RC_CHECKSUM = 3,
	LZMA_RC_UNSUPPORTED = 4,
	LZMA_RC_PARAMETER = 5,
	LZMA_RC_INPUT_EOF = 6,
	LZMA_RC_OUTPUT_EOF = 7,
	LZMA_RC_READ = 8,
	LZMA_RC_WRITE = 9,
	LZMA_RC_PROGRESS = 10,
	LZMA_RC_FAILURE = 11,
	LZMA_RC_THREAD = 12,
	LZMA_RC_ARCHIVE = 16,
	LZMA_RC_NO_ARCHIVE = 17
}
lzma_rc_t;


typedef struct lzma_byte_in_t lzma_byte_in_t;
struct lzma_byte_in_t { uint8_t (*read)(const lzma_byte_in_t* stream); };


typedef struct lzma_byte_out_t lzma_byte_out_t;
struct lzma_byte_out_t { void (*write)(const lzma_byte_out_t* stream, uint8_t b); };


typedef struct lzma_seq_in_stream_t lzma_seq_in_stream_t;
struct lzma_seq_in_stream_t { lzma_rc_t (*read)(const lzma_seq_in_stream_t* stream, void* buffer, size_t* size); };


lzma_rc_t lzma_seq_in_stream_read(const lzma_seq_in_stream_t* stream, void* buffer, size_t size);
lzma_rc_t lzma_seq_in_stream_read_ex(const lzma_seq_in_stream_t* stream, void* buffer, size_t size, lzma_rc_t error);
lzma_rc_t lzma_seq_in_stream_read_byte(const lzma_seq_in_stream_t* stream, uint8_t* buffer);


typedef struct lzma_seq_out_stream_t lzma_seq_out_stream_t;
struct lzma_seq_out_stream_t { size_t (*write)(const lzma_seq_out_stream_t* stream, const void* buffer, size_t size); };


typedef enum lzma_seek_e
{
	LZMA_SEEK_SET = 0,
	LZMA_SEEK_CUR = 1,
	LZMA_SEEK_END = 2
}
lzma_seek_t;


typedef struct lzma_seek_in_stream_t lzma_seek_in_stream_t;
struct lzma_seek_in_stream_t
{
	lzma_rc_t (*read)(const lzma_seek_in_stream_t* stream, void* buffer, size_t* size);
	lzma_rc_t (*seek)(const lzma_seek_in_stream_t* stream, int64_t* position, lzma_seek_t origin);
};


typedef struct lzma_look_in_stream_t lzma_look_in_stream_t;
struct lzma_look_in_stream_t
{
	lzma_rc_t (*look)(const lzma_look_in_stream_t* stream, const void** buffer, size_t* size);
	lzma_rc_t (*skip)(const lzma_look_in_stream_t* stream, size_t offset);
	lzma_rc_t (*read)(const lzma_look_in_stream_t* stream, void* buffer, size_t* size);
	lzma_rc_t (*seek)(const lzma_look_in_stream_t* stream, int64_t* position, lzma_seek_t origin);
};


lzma_rc_t lzma_look_in_stream_look_read(const lzma_look_in_stream_t* stream, void* buffer, size_t *size);
lzma_rc_t lzma_look_in_stream_seek_to(const lzma_look_in_stream_t* stream, uint64_t offset);
lzma_rc_t lzma_look_in_stream_read_ex(const lzma_look_in_stream_t* stream, void* buffer, size_t size, lzma_rc_t error);
lzma_rc_t lzma_look_in_stream_read(const lzma_look_in_stream_t* stream, void* buffer, size_t size);


typedef struct lzma_look_to_read_ex_s
{
	lzma_look_in_stream_t vtable;
	const lzma_seek_in_stream_t* base_stream;
	size_t position;
	size_t size;
	uint8_t* buffer;
	size_t buffer_size;
}
lzma_look_to_read_ex_t;


void lzma_look_to_read_ex_create_vtable(lzma_look_to_read_ex_t* state, int32_t lookahead);


typedef struct lzma_sec_to_look_s
{
	lzma_seq_in_stream_t vtable;
	const lzma_look_in_stream_t* base_stream;
}
lzma_sec_to_look_t;


void lzma_sec_to_look_create_vtable(lzma_sec_to_look_t* state);


typedef struct lzma_sec_to_read_s
{
	lzma_seq_in_stream_t vtable;
	const lzma_look_in_stream_t* base_stream;
}
lzma_sec_to_read_t;


void lzma_sec_to_read_create_vtable(lzma_sec_to_read_t* state);


typedef struct lzma_compress_progress_t lzma_compress_progress_t;
struct lzma_compress_progress_t { lzma_rc_t (*progress)(const lzma_compress_progress_t* progress, uint64_t in_size, uint64_t out_size); };


typedef struct lzma_allocator_t lzma_allocator_t;
typedef const lzma_allocator_t* lzma_allocator_ptr_t;
struct lzma_allocator_t
{
	void* (*allocate)(lzma_allocator_t* allocator, size_t size);
	void (*release)(lzma_allocator_t* allocator, void* address);
};


typedef struct lzma_encoder_properties_s
{
	int32_t level;
	uint32_t dictionary_size;
	int32_t lc;
	int32_t lp;
	int32_t pb;
	int32_t algorithm;
	int32_t fb;
	int32_t mode_bt;
	int32_t hash_byte_count;
	uint32_t mc;
	uint32_t write_end_mark;
	int32_t thread_count;
	uint64_t reduce_size;
}
lzma_encoder_properties_t;


void lzma_encoder_properties_init(lzma_encoder_properties_t* properties);
void lzma_encoder_properties_normalize(lzma_encoder_properties_t* properties);
uint32_t lzma_encoder_properties_dictionary_size(const lzma_encoder_properties_t* properties);


typedef void* lzma_encoder_handle_t;


lzma_encoder_handle_t lzma_encoder_create(lzma_allocator_ptr_t allocator);
void lzma_encoder_destroy(lzma_encoder_handle_t p, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large);
lzma_rc_t lzma_encoder_set_properties(lzma_encoder_handle_t p, const lzma_encoder_properties_t* properties);
void lzma_encoder_set_data_size(lzma_encoder_handle_t p, uint64_t expected_data_size);
lzma_rc_t lzma_encoder_write_properties(lzma_encoder_handle_t p, uint8_t* properties, size_t* size);
uint32_t lzma_encoder_is_write_end_mark(lzma_encoder_handle_t p);
lzma_rc_t lzma_encoder_encode(lzma_encoder_handle_t p, lzma_seq_out_stream_t* out_stream, lzma_seq_in_stream_t* in_stream, lzma_compress_progress_t* progress, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large);
lzma_rc_t lzma_encoder_encode_in_memory(lzma_encoder_handle_t p, uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t src_len, int32_t write_end_mark, lzma_compress_progress_t* progress, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large);
lzma_rc_t lzma_encode(uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t src_len, const lzma_encoder_properties_t* properties, uint8_t* properties_encoded, size_t* properties_size, int32_t write_end_mark, lzma_compress_progress_t* progress, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large);


typedef struct lzma_properties_s
{
	uint32_t lc, lp, pb;
	uint32_t dictionary_size;
}
lzma_properties_t;


lzma_rc_t lzma_properties_decode(lzma_properties_t* p, const uint8_t* data, uint32_t size);


typedef struct lzma_decoder_s
{
	lzma_properties_t properties;
	uint16_t* probabilities;
	uint8_t* dictionary;
	const uint8_t* buffer;
	uint32_t range, code;
	size_t dictionary_position;
	size_t dictionary_buffer_size;
	uint32_t processed_position;
	uint32_t check_dictionary_size;
	uint32_t state;
	uint32_t reps[4];
	uint32_t remaining_length;
	uint8_t need_to_flush;
	uint8_t need_to_init_state;
	uint32_t probabilities_count;
	uint32_t temp_buffer_size;
	uint8_t temp_buffer[20];
}
lzma_decoder_t;


void lzma_decoder_init(lzma_decoder_t* p);


typedef enum lzma_finish_mode_e
{
	LZMA_FINISH_ANY,
	LZMA_FINISH_END
}
lzma_finish_mode_t;


typedef enum lzma_status_e
{
	LZMA_STATUS_NOT_SPECIFIED,
	LZMA_STATUS_FINISHED_WITH_MARK,
	LZMA_STATUS_NOT_FINISHED,
	LZMA_STATUS_NEEDS_MORE_INPUT,
	LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK
}
lzma_status_t;


lzma_rc_t lzma_decoder_allocate_probabilities(lzma_decoder_t* p, const uint8_t* properties, uint32_t properties_size, lzma_allocator_ptr_t allocator);
void lzma_decoder_release_probabilities(lzma_decoder_t* p, lzma_allocator_ptr_t allocator);
lzma_rc_t lzma_decoder_allocate(lzma_decoder_t* state, const uint8_t* properties, uint32_t properties_size, lzma_allocator_ptr_t allocator);
void lzma_decoder_release(lzma_decoder_t* state, lzma_allocator_ptr_t allocator);
lzma_rc_t lzma_decoder_decode_to_dictionary(lzma_decoder_t* p, size_t dictionary_limit, const uint8_t* src, size_t* src_len, lzma_finish_mode_t finish_mode, lzma_status_t* status);
lzma_rc_t lzma_decoder_decode_to_buffer(lzma_decoder_t* p, uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t* src_len, lzma_finish_mode_t finish_mode, lzma_status_t* status);
lzma_rc_t lzma_decode(uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t* src_len, const uint8_t* properties_data, uint32_t properties_size, lzma_finish_mode_t finish_mode, lzma_status_t* status, lzma_allocator_ptr_t allocator);


typedef struct lzma_2_encoder_properties_s
{
	lzma_encoder_properties_t lzmaProps;
	uint64_t block_size;
	int32_t block_threads_reduced;
	int32_t block_threads_maximum;
	int32_t total_threads;
}
lzma_2_encoder_properties_t;


void lzma_2_encoder_properties_init(lzma_2_encoder_properties_t *p);
void lzma_2_encoder_properties_normalize(lzma_2_encoder_properties_t *p);


typedef void* lzma_2_encoder_handle_t;


lzma_2_encoder_handle_t lzma_2_encoder_create(lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large);
void lzma_2_encoder_destroy(lzma_2_encoder_handle_t handle);
lzma_rc_t lzma_2_encoder_set_properties(lzma_2_encoder_handle_t handle, const lzma_2_encoder_properties_t* properties);
void lzma_2_encoder_set_data_size(lzma_2_encoder_handle_t handle, uint64_t expected_data_size);
uint8_t lzma_2_encoder_write_properties(lzma_2_encoder_handle_t handle);
lzma_rc_t lzma_2_encoder_encode_ex(lzma_2_encoder_handle_t handle, lzma_seq_out_stream_t* out_stream, uint8_t* out_buffer, size_t* out_buffer_size, lzma_seq_in_stream_t* in_stream, const uint8_t* in_data, size_t in_data_size, lzma_compress_progress_t* progress);


typedef struct lzma_2_decoder_s
{
	lzma_decoder_t decoder;
	uint32_t packed_size;
	uint32_t unpacked_size;
	uint32_t state;
	uint8_t control;
	uint8_t need_to_init_dictionary;
	uint8_t need_to_init_state;
	uint8_t need_to_init_properties;
}
lzma_2_decoder_t;


lzma_rc_t lzma_2_decoder_allocate_probabilities(lzma_2_decoder_t* decoder, uint8_t properties, lzma_allocator_ptr_t allocator);
lzma_rc_t lzma_2_decoder_allocate(lzma_2_decoder_t* decoder, uint8_t properties, lzma_allocator_ptr_t allocator);
void lzma_2_decoder_init(lzma_2_decoder_t* decoder);
lzma_rc_t lzma_2_decoder_decode_to_dictionary(lzma_2_decoder_t* decoder, size_t dictionary_limit, const uint8_t* src, size_t* src_len, lzma_finish_mode_t finish_mode, lzma_status_t* status);
lzma_rc_t lzma_2_decoder_decode_to_buffer(lzma_2_decoder_t* decoder, uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t* src_len, lzma_finish_mode_t finish_mode, lzma_status_t* status);
lzma_rc_t lzma_2_decode(uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t* src_len, uint8_t properties, lzma_finish_mode_t finish_mode, lzma_status_t* status, lzma_allocator_ptr_t allocator);


typedef enum lzma_2_state_e
{
	LZMA2_STATE_CONTROL,
	LZMA2_STATE_UNPACK0,
	LZMA2_STATE_UNPACK1,
	LZMA2_STATE_PACK0,
	LZMA2_STATE_PACK1,
	LZMA2_STATE_PROP,
	LZMA2_STATE_DATA,
	LZMA2_STATE_DATA_CONT,
	LZMA2_STATE_FINISHED,
	LZMA2_STATE_ERROR
}
lzma_2_state_t;


inline static void* lzma_memset(void *restrict p, register uint8_t v, register size_t n)
{
	register uint8_t* d = (uint8_t*)p;
	while (n--) *d++ = v;
	return p;
}


inline static void* lzma_memmove(void *restrict dest, void const *restrict src, register size_t n)
{
	register uint8_t* d = (uint8_t*)dest;
	register uint8_t const* s = (uint8_t const*)src;

	if (d < s) while (n--) *d++ = *s++;
	else
	{
		d += n, s += n;
		while (n--) *--d = *--s;
	}

	return dest;
}


inline static void* lzma_memcpy(void *restrict p, const void *restrict q, size_t n)
{
	register const uint8_t* s;
	register uint8_t* d;

	if (p < q)
	{
		s = (const uint8_t*)q;
		d = (uint8_t*)p;

		while (n--) *d++ = *s++;

		return p;
	}

	s = (const uint8_t*)q + (n - UINT64_C(1));
	d = (uint8_t*)p + (n - UINT64_C(1));

	while (n--) *d-- = *s--;

	return p;
}


static int32_t  lzma_decoder_decode_real(lzma_decoder_t* p, size_t limit, const uint8_t* buffer_limit)
{
	uint16_t* probabilities = p->probabilities;
	uint32_t state = p->state;
	uint32_t rep_a = p->reps[0], rep_b = p->reps[1], rep_c = p->reps[2], rep_d = p->reps[3];
	uint32_t pb_mask = ((uint32_t)1 << (p->properties.pb)) - 1;
	uint32_t lp_mask = ((uint32_t)1 << (p->properties.lp)) - 1;
	uint32_t lc = p->properties.lc;
	uint8_t* dictionary = p->dictionary;
	size_t dictionary_buffer_size = p->dictionary_buffer_size;
	size_t dictionary_position = p->dictionary_position;
	uint32_t processed_position = p->processed_position;
	uint32_t check_dictionary_size = p->check_dictionary_size;
	uint32_t len = 0;
	const uint8_t* buffer = p->buffer;
	uint32_t range = p->range;
	uint32_t code = p->code;

	do
	{
		uint16_t* prob;
		uint32_t bound;
		uint32_t ttt;
		uint32_t pos_state = processed_position & pb_mask;

		prob = probabilities + 0 + (state << 4) + pos_state;

		ttt = *(prob); if (range < ((uint32_t)1 << 24)) { range <<= 8; code = (code << 8) | (*buffer++); }; bound = (range >> 11) * ttt; if (code < bound)
		{
			uint32_t symbol;
			range = bound; *(prob) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5));

			prob = probabilities + (((((((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + (1 << (14 >> 1)) - 14) + (1 << 4)) + (((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3)) + (1 << 8))) + (((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3)) + (1 << 8)));

			if (processed_position != 0 || check_dictionary_size != 0) prob += ((uint32_t)0x300 * (((processed_position & lp_mask) << lc) +
				(dictionary[(dictionary_position == 0 ? dictionary_buffer_size : dictionary_position) - 1] >> (8 - lc))));

			processed_position++;

			if (state < 7)
			{
				state -= (state < 4) ? state : 3;
				symbol = 1;

				do 
				{ 
					ttt = *(prob + symbol); 
					
					if (range < ((uint32_t)1 << 24)) 
					{ 
						range <<= 8; 
						code = (code << 8) | (*buffer++); 
					}

					bound = (range >> 11) * ttt; 
					
					if (code < bound) 
					{ 
						range = bound; 
						*(prob + symbol) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5));
						symbol = (symbol + symbol);
					} 
					else 
					{ 
						range -= bound; 
						code -= bound; 
						*(prob + symbol) = (uint16_t)(ttt - (ttt >> 5));
						symbol = (symbol + symbol) + 1;
					}
				} 
				while (symbol < 0x100);
			}
			else
			{
				uint32_t match_byte = dictionary[dictionary_position - rep_a + (dictionary_position < rep_a ? dictionary_buffer_size : 0)];
				uint32_t offs = 0x100;
				state -= (state < 10) ? 3 : 6;
				symbol = 1;

				do
				{
					uint32_t bit;
					uint16_t* prob_lit;

					match_byte <<= 1;
					bit = (match_byte & offs);
					prob_lit = prob + offs + bit + symbol;
					ttt = *(prob_lit);

					if (range < ((uint32_t)1 << 24))
					{
						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

					bound = (range >> 11) * ttt;

					if (code < bound)
					{
						range = bound; *(prob_lit) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5));
						symbol = (symbol + symbol);
						offs &= ~bit;
					}
					else
					{
						range -= bound;
						code -= bound;
						*(prob_lit) = (uint16_t)(ttt - (ttt >> 5));
						symbol = (symbol + symbol) + 1;
						offs &= bit;
					}
				}
				while (symbol < 0x100);
			}

			dictionary[dictionary_position++] = (uint8_t)symbol;

			continue;
		}

		{
			range -= bound;
			code -= bound; 
			*(prob) = (uint16_t)(ttt - (ttt >> 5));
			prob = probabilities + (0 + (12 << 4)) + state;
			ttt = *(prob);

			if (range < ((uint32_t)1 << 24))
			{
				range <<= 8;
				code = (code << 8) | (*buffer++);
			}

			bound = (range >> 11) * ttt;

			if (code < bound)
			{
				range = bound; *(prob) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5));
				state += 12;
				prob = probabilities + (((((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + (1 << (14 >> 1)) - 14) + (1 << 4));
			}
			else
			{
				range -= bound; 
				code -= bound; 
				*(prob) = (uint16_t)(ttt - (ttt >> 5));

				if (check_dictionary_size == 0 && processed_position == 0)
					return LZMA_RC_DATA;

				prob = probabilities + ((0 + (12 << 4)) + 12) + state;

				ttt = *(prob); 
				
				if (range < ((uint32_t)1 << 24)) 
				{
					range <<= 8; 
					code = (code << 8) | (*buffer++); 
				}
				
				bound = (range >> 11) * ttt;
				
				if (code < bound)
				{
					range = bound; *(prob) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5));

					prob = probabilities + (((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (state << 4) + pos_state;

					ttt = *(prob); 
					
					if (range < ((uint32_t)1 << 24)) 
					{ 
						range <<= 8; 
						code = (code << 8) | (*buffer++);
					}
					
					bound = (range >> 11) * ttt; 
					
					if (code < bound)
					{
						range = bound; *(prob) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5));

						dictionary[dictionary_position] = dictionary[dictionary_position - rep_a + (dictionary_position < rep_a ? dictionary_buffer_size : 0)];
						dictionary_position++;
						processed_position++;
						state = state < 7 ? 9 : 11;

						continue;
					}

					range -= bound; 
					code -= bound; 
					*(prob) = (uint16_t)(ttt - (ttt >> 5));
				}
				else
				{
					uint32_t distance;
					range -= bound; code -= bound; *(prob) = (uint16_t)(ttt - (ttt >> 5));
					prob = probabilities + (((0 + (12 << 4)) + 12) + 12) + state;
					ttt = *(prob); 
					
					if (range < ((uint32_t)1 << 24)) 
					{ 
						range <<= 8; 
						code = (code << 8) | (*buffer++);
					}
					
					bound = (range >> 11) * ttt; 
					
					if (code < bound)
					{
						range = bound; *(prob) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5));
						distance = rep_b;
					}
					else
					{
						range -= bound; code -= bound; *(prob) = (uint16_t)(ttt - (ttt >> 5));
						prob = probabilities + ((((0 + (12 << 4)) + 12) + 12) + 12) + state;
						ttt = *(prob); 
						
						if (range < ((uint32_t)1 << 24)) 
						{ 
							range <<= 8; 
							code = (code << 8) | (*buffer++); 
						}
						
						bound = (range >> 11) * ttt; 
						
						if (code < bound)
						{
							range = bound; *(prob) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5));
							distance = rep_c;
						}
						else
						{
							range -= bound; code -= bound; *(prob) = (uint16_t)(ttt - (ttt >> 5));
							distance = rep_d;
							rep_d = rep_c;
						}

						rep_c = rep_b;
					}

					rep_b = rep_a;
					rep_a = distance;
				}

				state = state < 7 ? 8 : 11;
				prob = probabilities + ((((((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + (1 << (14 >> 1)) - 14) + (1 << 4)) + (((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3)) + (1 << 8)));
			}


			{
				uint32_t lim, offset;
				uint16_t* prob_len = prob + 0;

				ttt = *(prob_len); 
				
				if (range < ((uint32_t)1 << 24)) 
				{ 
					range <<= 8; 
					code = (code << 8) | (*buffer++); 
				}
				
				bound = (range >> 11) * ttt; 
				
				if (code < bound)
				{
					range = bound; *(prob_len) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5));
					prob_len = prob + ((0 + 1) + 1) + (pos_state << 3);
					offset = 0;
					lim = (1 << 3);
				}
				else
				{
					range -= bound; code -= bound; *(prob_len) = (uint16_t)(ttt - (ttt >> 5));
					prob_len = prob + 1;
					ttt = *(prob_len); 
					
					if (range < ((uint32_t)1 << 24)) 
					{ 
						range <<= 8; 
						code = (code << 8) | (*buffer++); 
					}
					
					bound = (range >> 11) * ttt; 
					
					if (code < bound)
					{
						range = bound; 
						*(prob_len) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5));
						prob_len = prob + (((0 + 1) + 1) + ((1 << 4) << 3)) + (pos_state << 3);
						offset = (1 << 3);
						lim = (1 << 3);
					}
					else
					{
						range -= bound; code -= bound; *(prob_len) = (uint16_t)(ttt - (ttt >> 5));
						prob_len = prob + ((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3));
						offset = (1 << 3) + (1 << 3);
						lim = (1 << 8);
					}
				}

				{
					len = 1; 
					
					do 
					{ 
						{ 
							ttt = *((prob_len + len)); 
							
							if (range < ((uint32_t)1 << 24)) 
							{ 
								range <<= 8; 
								code = (code << 8) | (*buffer++); 
							}
							
							bound = (range >> 11) * ttt; 
							
							if (code < bound) 
							{ 
								range = bound; 
								*((prob_len + len)) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5)); 
								len = (len + len);
							} 
							else 
							{ 
								range -= bound; 
								code -= bound; 
								*((prob_len + len)) = (uint16_t)(ttt - (ttt >> 5)); 
								len = (len + len) + 1;
							}
						}
					} 
					while (len < lim); 
					
					len -= lim; 
				}

				len += offset;
			}

			if (state >= 12)
			{
				uint32_t distance;
				prob = probabilities + ((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + ((len < 4 ? len : 4 - 1) << 6);

				{ 
					distance = 1;

					do 
					{ 
						{ 
							ttt = *((prob + distance)); 
							
							if (range < ((uint32_t)1 << 24)) 
							{ 
								range <<= 8; 
								code = (code << 8) | (*buffer++); 
							}
							
							bound = (range >> 11) * ttt; 
							
							if (code < bound) 
							{ 
								range = bound; 
								*((prob + distance)) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5)); 
								distance = (distance + distance);
							} 
							else 
							{ 
								range -= bound; 
								code -= bound; 
								*((prob + distance)) = (uint16_t)(ttt - (ttt >> 5)); 
								distance = (distance + distance) + 1;
							}
						}
					} 
					while (distance < (1 << 6)); 
					
					distance -= (1 << 6); 
				}

				if (distance >= 4)
				{
					uint32_t pos_slot = (uint32_t)distance;
					uint32_t nr_direct_bits = (uint32_t)(((distance >> 1) - 1));

					distance = (2 | (distance & 1));

					if (pos_slot < 14)
					{
						distance <<= nr_direct_bits;
						prob = probabilities + (((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + distance - pos_slot - 1;

						{
							uint32_t mask = 1;
							uint32_t i = 1;

							do
							{
								ttt = *(prob + i); 
								
								if (range < ((uint32_t)1 << 24)) 
								{ 
									range <<= 8;
									code = (code << 8) | (*buffer++); 
								}
								
								bound = (range >> 11) * ttt; 
								
								if (code < bound) 
								{ 
									range = bound; 
									*(prob + i) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5)); 
									i = (i + i);
								}
								else 
								{ 
									range -= bound; 
									code -= bound; 
									*(prob + i) = (uint16_t)(ttt - (ttt >> 5)); 
									i = (i + i) + 1; 
									distance |= mask;
								}

								mask <<= 1;
							} 
							while (--nr_direct_bits != 0);
						}
					}
					else
					{
						nr_direct_bits -= 4;

						do
						{
							if (range < ((uint32_t)1 << 24)) 
							{ 
								range <<= 8; 
								code = (code << 8) | (*buffer++); 
							}

							range >>= 1;

							{
								uint32_t t;
								code -= range;
								t = (0 - ((uint32_t)code >> 31));
								distance = (distance << 1) + (t + 1);
								code += range & t;
							}
						}  
						while (--nr_direct_bits != 0);

						prob = probabilities + ((((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + (1 << (14 >> 1)) - 14);
						distance <<= 4;

						{
							uint32_t i = 1;

							ttt = *(prob + i); 
							
							if (range < ((uint32_t)1 << 24)) { range <<= 8; code = (code << 8) | (*buffer++); }
							bound = (range >> 11) * ttt; 
							
							if (code < bound) { range = bound; *(prob + i) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5)); i = (i + i); }
							else { range -= bound; code -= bound; *(prob + i) = (uint16_t)(ttt - (ttt >> 5)); i = (i + i) + 1; distance |= 1; }
							
							ttt = *(prob + i); 
							
							if (range < ((uint32_t)1 << 24)) { range <<= 8; code = (code << 8) | (*buffer++); }
							bound = (range >> 11) * ttt; 
							
							if (code < bound) { range = bound; *(prob + i) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5)); i = (i + i); }
							else { range -= bound; code -= bound; *(prob + i) = (uint16_t)(ttt - (ttt >> 5)); i = (i + i) + 1; distance |= 2; }
							
							ttt = *(prob + i); 
							
							if (range < ((uint32_t)1 << 24)) { range <<= 8; code = (code << 8) | (*buffer++); }
							bound = (range >> 11) * ttt; 
							
							if (code < bound) { range = bound; *(prob + i) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5)); i = (i + i); }
							else { range -= bound; code -= bound; *(prob + i) = (uint16_t)(ttt - (ttt >> 5)); i = (i + i) + 1; distance |= 4; }
							
							ttt = *(prob + i); 
							
							if (range < ((uint32_t)1 << 24)) { range <<= 8; code = (code << 8) | (*buffer++); }
							bound = (range >> 11) * ttt; 
							
							if (code < bound) { range = bound; *(prob + i) = (uint16_t)(ttt + (((1 << 11) - ttt) >> 5)); i = (i + i); }
							else { range -= bound; code -= bound; *(prob + i) = (uint16_t)(ttt - (ttt >> 5)); i = (i + i) + 1; distance |= 8; }
						}

						if (distance == UINT32_C(0xFFFFFFFF))
						{
							len += (2 + (1 << 3) + (1 << 3) + (1 << 8));
							state -= 12;
							break;
						}
					}
				}

				rep_d = rep_c;
				rep_c = rep_b;
				rep_b = rep_a;
				rep_a = distance + 1;

				if (check_dictionary_size == 0)
				{
					if (distance >= processed_position)
					{
						p->dictionary_position = dictionary_position;

						return LZMA_RC_DATA;
					}
				}
				else if (distance >= check_dictionary_size)
				{
					p->dictionary_position = dictionary_position;

					return LZMA_RC_DATA;
				}

				state = (state < 12 + 7) ? 7 : 7 + 3;
			}

			len += 2;

			{
				size_t rem;
				uint32_t curr_len;
				size_t pos;

				if ((rem = limit - dictionary_position) == 0)
				{
					p->dictionary_position = dictionary_position;

					return LZMA_RC_DATA;
				}

				curr_len = ((rem < len) ? (uint32_t)rem : len);
				pos = dictionary_position - rep_a + (dictionary_position < rep_a ? dictionary_buffer_size : 0);

				processed_position += curr_len;

				len -= curr_len;

				if (curr_len <= dictionary_buffer_size - pos)
				{
					uint8_t* dest = dictionary + dictionary_position;
					ptrdiff_t src = (ptrdiff_t)pos - (ptrdiff_t)dictionary_position;
					const uint8_t *lim = dest + curr_len;
					dictionary_position += curr_len;

					do *(dest) = (uint8_t)*(dest + src);
					while (++dest != lim);
				}
				else
				{
					do
					{
						dictionary[dictionary_position++] = dictionary[pos];

						if (++pos == dictionary_buffer_size)
							pos = 0;
					} 
					while (--curr_len != 0);
				}
			}
		}
	} 
	while (dictionary_position < limit && buffer < buffer_limit);

	if (range < ((uint32_t)1 << 24)) { range <<= 8; code = (code << 8) | (*buffer++); }

	p->buffer = buffer;
	p->range = range;
	p->code = code;
	p->remaining_length = len;
	p->dictionary_position = dictionary_position;
	p->processed_position = processed_position;
	p->reps[0] = rep_a;
	p->reps[1] = rep_b;
	p->reps[2] = rep_c;
	p->reps[3] = rep_d;
	p->state = state;

	return LZMA_RC_OK;
}


static void  lzma_decoder_write_rem(lzma_decoder_t *p, size_t limit)
{
	if (p->remaining_length != 0 && p->remaining_length < (2 + (1 << 3) + (1 << 3) + (1 << 8)))
	{
		uint8_t *dictionary = p->dictionary;
		size_t dictionary_position = p->dictionary_position;
		size_t dictionary_buffer_size = p->dictionary_buffer_size;
		uint32_t len = p->remaining_length;
		size_t rep_a = p->reps[0];
		size_t rem = limit - dictionary_position;

		if (rem < len)
			len = (uint32_t)(rem);

		if (p->check_dictionary_size == 0 && p->properties.dictionary_size - p->processed_position <= len)
			p->check_dictionary_size = p->properties.dictionary_size;

		p->processed_position += len;
		p->remaining_length -= len;

		while (len != 0)
		{
			len--;
			dictionary[dictionary_position] = dictionary[dictionary_position - rep_a + (dictionary_position < rep_a ? dictionary_buffer_size : 0)];
			dictionary_position++;
		}

		p->dictionary_position = dictionary_position;
	}
}


static int32_t lzma_decoder_decode_real_ex(lzma_decoder_t *p, size_t limit, const uint8_t *buffer_limit)
{
	do
	{
		size_t limit_b = limit;

		if (p->check_dictionary_size == 0)
		{
			uint32_t rem = p->properties.dictionary_size - p->processed_position;

			if (limit - p->dictionary_position > rem)
				limit_b = p->dictionary_position + rem;
		}

		{ lzma_rc_t rc__ = (lzma_decoder_decode_real(p, limit_b, buffer_limit)); if (rc__ != 0) return rc__; };

		if (p->check_dictionary_size == 0 && p->processed_position >= p->properties.dictionary_size)
			p->check_dictionary_size = p->properties.dictionary_size;

		lzma_decoder_write_rem(p, limit);
	} 
	while (p->dictionary_position < limit && p->buffer < buffer_limit && p->remaining_length < (2 + (1 << 3) + (1 << 3) + (1 << 8)));

	if (p->remaining_length > (2 + (1 << 3) + (1 << 3) + (1 << 8)))
		p->remaining_length = (2 + (1 << 3) + (1 << 3) + (1 << 8));

	return 0;
}


typedef enum lzma_dummy_e
{
	LZMA_DUMMY_ERROR,
	LZMA_DUMMY_LIT,
	LZMA_DUMMY_MATCH,
	LZMA_DUMMY_REP
} 
lzma_dummy_t;


static lzma_dummy_t lzma_decoder_try_dummy(const lzma_decoder_t *p, const uint8_t *buffer, size_t in_size)
{
	uint32_t range = p->range;
	uint32_t code = p->code;
	const uint8_t* buffer_limit = buffer + in_size;
	const uint16_t* probabilities = p->probabilities;
	uint32_t state = p->state;
	lzma_dummy_t res;

	{
		const uint16_t *prob;
		uint32_t bound;
		uint32_t ttt;
		uint32_t pos_state = (p->processed_position) & ((1 << p->properties.pb) - 1);

		prob = probabilities + 0 + (state << 4) + pos_state;

		ttt = *(prob); 
		
		if (range < ((uint32_t)1 << 24)) 
		{ 
			if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; 
			range <<= 8; 
			code = (code << 8) | (*buffer++); 
		}
		
		bound = (range >> 11) * ttt;
		
		if (code < bound)
		{
			range = bound;

			prob = probabilities + (((((((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + (1 << (14 >> 1)) - 14) + (1 << 4)) + (((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3)) + (1 << 8))) + (((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3)) + (1 << 8)));

			if (p->check_dictionary_size != 0 || p->processed_position != 0)
				prob += ((uint32_t)0x300 * ((((p->processed_position) & ((1 << (p->properties.lp)) - 1)) << p->properties.lc) +
				(p->dictionary[(p->dictionary_position == 0 ? p->dictionary_buffer_size : p->dictionary_position) - 1] >> (8 - p->properties.lc))));

			if (state < 7)
			{
				uint32_t symbol = 1;

				do 
				{ 
					ttt = *(prob + symbol); 
					
					if (range < ((uint32_t)1 << 24)) 
					{ 
						if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; 
						range <<= 8; 
						code = (code << 8) | (*buffer++); 
					}
					
					bound = (range >> 11) * ttt; 
					
					if (code < bound) 
					{ 
						range = bound; 
						symbol = (symbol + symbol); 
					} 
					else 
					{ 
						range -= bound; 
						code -= bound; 
						symbol = (symbol + symbol) + 1; 
					}
				} 
				while (symbol < 0x100);
			}
			else
			{
				uint32_t match_byte = p->dictionary[p->dictionary_position - p->reps[0] + (p->dictionary_position < p->reps[0] ? p->dictionary_buffer_size : 0)];
				uint32_t offs = 0x100;
				uint32_t symbol = 1;

				do
				{
					uint32_t bit;
					const uint16_t* prob_lit;
					match_byte <<= 1;
					bit = (match_byte & offs);
					prob_lit = prob + offs + bit + symbol;
					ttt = *(prob_lit); 
					
					if (range < ((uint32_t)1 << 24)) 
					{ 
						if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; 
						range <<= 8; code = (code << 8) | (*buffer++); 
					}
					
					bound = (range >> 11) * ttt; 

					if (code < bound) { range = bound; symbol = (symbol + symbol); offs &= ~bit; }
					else { range -= bound; code -= bound; symbol = (symbol + symbol) + 1; offs &= bit; }
				} 
				while (symbol < 0x100);
			}

			res = LZMA_DUMMY_LIT;
		}
		else
		{
			uint32_t len;

			range -= bound; code -= bound;

			prob = probabilities + (0 + (12 << 4)) + state;

			ttt = *(prob); 
			
			if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }
			
			bound = (range >> 11) * ttt; 
			
			if (code < bound)
			{
				range = bound;
				state = 0;
				prob = probabilities + (((((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + (1 << (14 >> 1)) - 14) + (1 << 4));
				res = LZMA_DUMMY_MATCH;
			}
			else
			{
				range -= bound; code -= bound;
				res = LZMA_DUMMY_REP;
				prob = probabilities + ((0 + (12 << 4)) + 12) + state;

				ttt = *(prob); 
				
				if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }
				
				bound = (range >> 11) * ttt; 
				
				if (code < bound)
				{
					range = bound;
					prob = probabilities + (((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (state << 4) + pos_state;

					ttt = *(prob); 
					
					if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }
					
					bound = (range >> 11) * ttt; 
					
					if (code < bound)
					{
						range = bound;

						if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }

						return LZMA_DUMMY_REP;
					}
					else
					{
						range -= bound; 
						code -= bound;
					}
				}
				else
				{
					range -= bound; code -= bound;
					prob = probabilities + (((0 + (12 << 4)) + 12) + 12) + state;

					ttt = *(prob); 
					
					if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }
					
					bound = (range >> 11) * ttt; 
					
					if (code < bound)
					{
						range = bound;
					}
					else
					{
						range -= bound; code -= bound;
						prob = probabilities + ((((0 + (12 << 4)) + 12) + 12) + 12) + state;

						ttt = *(prob); 
						
						if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }
						
						bound = (range >> 11) * ttt; 
						
						if (code < bound)
							range = bound;
						else
						{
							range -= bound; 
							code -= bound;
						}
					}
				}

				state = 12;
				prob = probabilities + ((((((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + (1 << (14 >> 1)) - 14) + (1 << 4)) + (((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3)) + (1 << 8)));
			}

			{
				uint32_t limit, offset;
				const uint16_t* prob_len = prob + 0;

				ttt = *(prob_len); 
				
				if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }
				
				bound = (range >> 11) * ttt; 
				
				if (code < bound)
				{
					range = bound;
					prob_len = prob + ((0 + 1) + 1) + (pos_state << 3);
					offset = 0;
					limit = 1 << 3;
				}
				else
				{
					range -= bound; code -= bound;
					prob_len = prob + (0 + 1);

					ttt = *(prob_len); 
					
					if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }
					
					bound = (range >> 11) * ttt; 
					
					if (code < bound)
					{
						range = bound;
						prob_len = prob + (((0 + 1) + 1) + ((1 << 4) << 3)) + (pos_state << 3);
						offset = (1 << 3);
						limit = 1 << 3;
					}
					else
					{
						range -= bound; code -= bound;
						prob_len = prob + ((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3));
						offset = (1 << 3) + (1 << 3);
						limit = 1 << 8;
					}
				}

				{ 
					len = 1; 

					do 
					{ 
						ttt = *(prob_len + len); 
						if (range < ((uint32_t)1 << 24)) 
						{ 
							if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; 
							range <<= 8; code = (code << 8) | (*buffer++); 
						}
						
						bound = (range >> 11) * ttt; 
						
						if (code < bound) { range = bound; len = (len + len); } 
						else { range -= bound; code -= bound; len = (len + len) + 1; } 
					} 
					while (len < limit); len -= limit; 
				}

				len += offset;
			}

			if (state < 4)
			{
				uint32_t pos_slot;

				prob = probabilities + ((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + ((len < 4 ? len : 4 - 1) << 6);

				{ 
					pos_slot = 1; 
					
					do 
					{ 
						ttt = *(prob + pos_slot); 
						
						if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }
						
						bound = (range >> 11) * ttt; 
						
						if (code < bound) { range = bound; pos_slot = (pos_slot + pos_slot); } 
						else { range -= bound; code -= bound; pos_slot = (pos_slot + pos_slot) + 1; }
					} 
					while (pos_slot < 1 << 6); 
					
					pos_slot -= 1 << 6; 
				}

				if (pos_slot >= 4)
				{
					uint32_t nr_direct_bits = ((pos_slot >> 1) - 1);

					if (pos_slot < 14)
					{
						prob = probabilities + (((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + ((2 | (pos_slot & 1)) << nr_direct_bits) - pos_slot - 1;
					}
					else
					{
						nr_direct_bits -= 4;
						do
						{
							if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }
							range >>= 1;
							code -= range & (((code - range) >> 31) - 1);

						} 
						while (--nr_direct_bits != 0);

						prob = probabilities + ((((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + (1 << (14 >> 1)) - 14);
						nr_direct_bits = 4;
					}

					{
						uint32_t i = 1;
						do
						{
							ttt = *(prob + i); 
							
							if (range < ((uint32_t)1 << 24)) { if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buffer++); }
							
							bound = (range >> 11) * ttt; 
							
							if (code < bound) { range = bound; i = (i + i); }
							else { range -= bound; code -= bound; i = (i + i) + 1; }
						} 
						while (--nr_direct_bits != 0);
					}
				}
			}
		}
	}

	if (range < ((uint32_t)1 << 24)) 
	{ 
		if (buffer >= buffer_limit) return LZMA_DUMMY_ERROR; 
		range <<= 8; code = (code << 8) | (*buffer++); 
	}

	return res;
}


void lzma_decoder_init_dictionary_and_state(lzma_decoder_t *p, uint8_t init_dictionary, uint8_t init_state)
{
	p->need_to_flush = 1;
	p->remaining_length = 0;
	p->temp_buffer_size = 0;

	if (init_dictionary)
	{
		p->processed_position = 0;
		p->check_dictionary_size = 0;
		p->need_to_init_state = 1;
	}

	if (init_state)
		p->need_to_init_state = 1;
}


void lzma_decoder_init(lzma_decoder_t *p)
{
	p->dictionary_position = 0;
	lzma_decoder_init_dictionary_and_state(p, 1, 1);
}


static void lzma_decoder_init_state_real(lzma_decoder_t *p)
{
	size_t probabilities_count = ((((((((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + (1 << (14 >> 1)) - 14) + (1 << 4)) + (((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3)) + (1 << 8))) + (((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3)) + (1 << 8))) + ((uint32_t)0x300 << ((&p->properties)->lc + (&p->properties)->lp)));
	size_t i;
	uint16_t* probabilities = p->probabilities;

	for (i = 0; i < probabilities_count; ++i)
		probabilities[i] = (1 << 11) >> 1;

	p->reps[0] = p->reps[1] = p->reps[2] = p->reps[3] = 1;
	p->state = 0;
	p->need_to_init_state = 0;
}


lzma_rc_t lzma_decoder_decode_to_dictionary(lzma_decoder_t *p, size_t dictionary_limit, const uint8_t *src, size_t *src_len, lzma_finish_mode_t finish_mode, lzma_status_t *status)
{
	size_t in_size = *src_len;

	(*src_len) = 0;

	lzma_decoder_write_rem(p, dictionary_limit);

	*status = LZMA_STATUS_NOT_SPECIFIED;

	while (p->remaining_length != (2 + (1 << 3) + (1 << 3) + (1 << 8)))
	{
		int32_t check_end_mark_now;

		if (p->need_to_flush)
		{
			for (; in_size > 0 && p->temp_buffer_size < 5; (*src_len)++, in_size--)
				p->temp_buffer[p->temp_buffer_size++] = *src++;

			if (p->temp_buffer_size < 5)
			{
				*status = LZMA_STATUS_NEEDS_MORE_INPUT;

				return LZMA_RC_OK;
			}

			if (p->temp_buffer[0] != 0)
				return LZMA_RC_DATA;

			p->code = ((uint32_t)p->temp_buffer[1] << 24) | ((uint32_t)p->temp_buffer[2] << 16) | ((uint32_t)p->temp_buffer[3] << 8) | ((uint32_t)p->temp_buffer[4]);

			p->range = UINT32_C(0xFFFFFFFF);
			p->need_to_flush = 0;
			p->temp_buffer_size = 0;
		}

		check_end_mark_now = 0;

		if (p->dictionary_position >= dictionary_limit)
		{
			if (p->remaining_length == 0 && p->code == 0)
			{
				*status = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK;

				return LZMA_RC_OK;
			}

			if (finish_mode == LZMA_FINISH_ANY)
			{
				*status = LZMA_STATUS_NOT_FINISHED;

				return LZMA_RC_OK;
			}

			if (p->remaining_length != 0)
			{
				*status = LZMA_STATUS_NOT_FINISHED;

				return LZMA_RC_DATA;
			}

			check_end_mark_now = 1;
		}

		if (p->need_to_init_state)
			lzma_decoder_init_state_real(p);

		if (p->temp_buffer_size == 0)
		{
			size_t processed;
			const uint8_t* buffer_limit;

			if (in_size < 20 || check_end_mark_now)
			{
				lzma_dummy_t dummy_rc = lzma_decoder_try_dummy(p, src, in_size);

				if (dummy_rc == LZMA_DUMMY_ERROR)
				{
					lzma_memcpy(p->temp_buffer, src, in_size);
					p->temp_buffer_size = (uint32_t)in_size;
					(*src_len) += in_size;
					*status = LZMA_STATUS_NEEDS_MORE_INPUT;

					return LZMA_RC_OK;
				}

				if (check_end_mark_now && dummy_rc != LZMA_DUMMY_MATCH)
				{
					*status = LZMA_STATUS_NOT_FINISHED;

					return LZMA_RC_DATA;
				}

				buffer_limit = src;
			}
			else buffer_limit = src + in_size - 20;

			p->buffer = src;

			if (lzma_decoder_decode_real_ex(p, dictionary_limit, buffer_limit) != 0)
				return LZMA_RC_DATA;

			processed = (size_t)(p->buffer - src);
			(*src_len) += processed;
			src += processed;
			in_size -= processed;
		}
		else
		{
			uint32_t rem = p->temp_buffer_size, look_ahead = 0;

			while (rem < 20 && look_ahead < in_size)
				p->temp_buffer[rem++] = src[look_ahead++];

			p->temp_buffer_size = rem;

			if (rem < 20 || check_end_mark_now)
			{
				lzma_dummy_t dummy_rc = lzma_decoder_try_dummy(p, p->temp_buffer, rem);

				if (dummy_rc == LZMA_DUMMY_ERROR)
				{
					(*src_len) += look_ahead;
					*status = LZMA_STATUS_NEEDS_MORE_INPUT;

					return LZMA_RC_OK;
				}

				if (check_end_mark_now && dummy_rc != LZMA_DUMMY_MATCH)
				{
					*status = LZMA_STATUS_NOT_FINISHED;

					return LZMA_RC_DATA;
				}
			}

			p->buffer = p->temp_buffer;

			if (lzma_decoder_decode_real_ex(p, dictionary_limit, p->buffer) != 0)
				return LZMA_RC_DATA;

			{
				uint32_t kkk = (uint32_t)(p->buffer - p->temp_buffer);

				if (rem < kkk)
					return LZMA_RC_FAILURE;

				rem -= kkk;

				if (look_ahead < rem)
					return LZMA_RC_FAILURE;

				look_ahead -= rem;
			}

			(*src_len) += look_ahead;
			src += look_ahead;
			in_size -= look_ahead;
			p->temp_buffer_size = 0;
		}
	}

	if (p->code == 0)
		*status = LZMA_STATUS_FINISHED_WITH_MARK;

	return (p->code == 0) ? LZMA_RC_OK : LZMA_RC_DATA;
}


lzma_rc_t lzma_decoder_decode_to_buffer(lzma_decoder_t *p, uint8_t *dest, size_t *dest_len, const uint8_t *src, size_t *src_len, lzma_finish_mode_t finish_mode, lzma_status_t *status)
{
	size_t out_size = *dest_len;
	size_t in_size = *src_len;

	*src_len = *dest_len = 0;

	for (;;)
	{
		size_t inSizeCur = in_size, outSizeCur, dictionary_position;
		lzma_finish_mode_t curFinishMode;
		lzma_rc_t res;

		if (p->dictionary_position == p->dictionary_buffer_size)
			p->dictionary_position = 0;

		dictionary_position = p->dictionary_position;

		if (out_size > p->dictionary_buffer_size - dictionary_position)
		{
			outSizeCur = p->dictionary_buffer_size;
			curFinishMode = LZMA_FINISH_ANY;
		}
		else
		{
			outSizeCur = dictionary_position + out_size;
			curFinishMode = finish_mode;
		}

		res = lzma_decoder_decode_to_dictionary(p, outSizeCur, src, &inSizeCur, curFinishMode, status);

		src += inSizeCur;
		in_size -= inSizeCur;
		*src_len += inSizeCur;
		outSizeCur = p->dictionary_position - dictionary_position;

		lzma_memcpy(dest, p->dictionary + dictionary_position, outSizeCur);

		dest += outSizeCur;
		out_size -= outSizeCur;
		*dest_len += outSizeCur;

		if (res != 0)
			return res;

		if (outSizeCur == 0 || out_size == 0)
			return LZMA_RC_OK;
	}
}


void lzma_decoder_release_probabilities(lzma_decoder_t *p, lzma_allocator_ptr_t allocator)
{
	(allocator)->release(allocator, p->probabilities);
	p->probabilities = ((void *)0);
}


static void LzmaDec_FreeDict(lzma_decoder_t *p, lzma_allocator_ptr_t allocator)
{
	(allocator)->release(allocator, p->dictionary);
	p->dictionary = ((void *)0);
}


void lzma_decoder_release(lzma_decoder_t *p, lzma_allocator_ptr_t allocator)
{
	lzma_decoder_release_probabilities(p, allocator);
	LzmaDec_FreeDict(p, allocator);
}


lzma_rc_t lzma_properties_decode(lzma_properties_t *p, const uint8_t *data, uint32_t size)
{
	uint32_t dictionary_size;
	uint8_t d;

	if (size < 5)
		return LZMA_RC_UNSUPPORTED;
	else dictionary_size = data[1] | ((uint32_t)data[2] << 8) | ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 24);

	if (dictionary_size < (1 << 12))
		dictionary_size = (1 << 12);

	p->dictionary_size = dictionary_size;

	d = data[0];

	if (d >= (9 * 5 * 5))
		return LZMA_RC_UNSUPPORTED;

	p->lc = d % 9;
	d /= 9;
	p->pb = d / 5;
	p->lp = d % 5;

	return LZMA_RC_OK;
}


static lzma_rc_t LzmaDec_AllocateProbs2(lzma_decoder_t *p, const lzma_properties_t *propNew, lzma_allocator_ptr_t allocator)
{
	uint32_t probabilities_count = ((((((((((((0 + (12 << 4)) + 12) + 12) + 12) + 12) + (12 << 4)) + (4 << 6)) + (1 << (14 >> 1)) - 14) + (1 << 4)) + (((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3)) + (1 << 8))) + (((((0 + 1) + 1) + ((1 << 4) << 3)) + ((1 << 4) << 3)) + (1 << 8))) + ((uint32_t)0x300 << ((propNew)->lc + (propNew)->lp)));

	if (!p->probabilities || probabilities_count != p->probabilities_count)
	{
		lzma_decoder_release_probabilities(p, allocator);

		p->probabilities = (uint16_t *)(allocator)->allocate(allocator, probabilities_count * sizeof(uint16_t));
		p->probabilities_count = probabilities_count;

		if (!p->probabilities)
			return LZMA_RC_MEMORY;
	}

	return LZMA_RC_OK;
}


lzma_rc_t lzma_decoder_allocate_probabilities(lzma_decoder_t *p, const uint8_t *properties, uint32_t properties_size, lzma_allocator_ptr_t allocator)
{
	lzma_properties_t propNew;

	{ lzma_rc_t rc__ = (lzma_properties_decode(&propNew, properties, properties_size)); if (rc__ != 0) return rc__; };
	{ lzma_rc_t rc__ = (LzmaDec_AllocateProbs2(p, &propNew, allocator)); if (rc__ != 0) return rc__; };
	p->properties = propNew;

	return LZMA_RC_OK;
}


lzma_rc_t lzma_decoder_allocate(lzma_decoder_t *p, const uint8_t *properties, uint32_t properties_size, lzma_allocator_ptr_t allocator)
{
	lzma_properties_t propNew;
	size_t dictionary_buffer_size;

	{ lzma_rc_t rc__ = (lzma_properties_decode(&propNew, properties, properties_size)); if (rc__ != 0) return rc__; };
	{ lzma_rc_t rc__ = (LzmaDec_AllocateProbs2(p, &propNew, allocator)); if (rc__ != 0) return rc__; };

	{
		uint32_t dictionary_size = propNew.dictionary_size;
		size_t mask = ((uint32_t)1 << 12) - 1;

		if (dictionary_size >= ((uint32_t)1 << 30)) mask = ((uint32_t)1 << 22) - 1;
		else if (dictionary_size >= ((uint32_t)1 << 22)) mask = ((uint32_t)1 << 20) - 1;

		dictionary_buffer_size = ((size_t)dictionary_size + mask) & ~mask;

		if (dictionary_buffer_size < dictionary_size)
			dictionary_buffer_size = dictionary_size;
	}

	if (!p->dictionary || dictionary_buffer_size != p->dictionary_buffer_size)
	{
		LzmaDec_FreeDict(p, allocator);

		p->dictionary = (uint8_t *)(allocator)->allocate(allocator, dictionary_buffer_size);

		if (!p->dictionary)
		{
			lzma_decoder_release_probabilities(p, allocator);

			return LZMA_RC_MEMORY;
		}
	}

	p->dictionary_buffer_size = dictionary_buffer_size;
	p->properties = propNew;

	return LZMA_RC_OK;
}


lzma_rc_t lzma_decode(uint8_t *dest, size_t *dest_len, const uint8_t *src, size_t *src_len, const uint8_t *properties_data, uint32_t properties_size, lzma_finish_mode_t finish_mode, lzma_status_t *status, lzma_allocator_ptr_t allocator)
{
	lzma_decoder_t p;
	lzma_rc_t res;
	size_t out_size = *dest_len, in_size = *src_len;

	*dest_len = *src_len = 0;
	*status = LZMA_STATUS_NOT_SPECIFIED;

	if (in_size < 5)
		return LZMA_RC_INPUT_EOF;

	{ (&p)->dictionary = 0; (&p)->probabilities = 0; };
	{ lzma_rc_t rc__ = (lzma_decoder_allocate_probabilities(&p, properties_data, properties_size, allocator)); if (rc__ != 0) return rc__; };

	p.dictionary = dest;
	p.dictionary_buffer_size = out_size;

	lzma_decoder_init(&p);

	*src_len = in_size;

	res = lzma_decoder_decode_to_dictionary(&p, out_size, src, src_len, finish_mode, status);

	*dest_len = p.dictionary_position;

	if (res == LZMA_RC_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT)
		res = LZMA_RC_INPUT_EOF;

	lzma_decoder_release_probabilities(&p, allocator);

	return res;
}


static lzma_rc_t Lzma2Dec_GetOldProps(uint8_t properties, uint8_t* buffer)
{
	uint32_t dictionary_size;

	if (properties > 40)
		return LZMA_RC_UNSUPPORTED;

	dictionary_size = (properties == 40) ? 0xFFFFFFFF : (((uint32_t)2 | ((properties) & 1)) << ((properties) / 2 + 11));

	buffer[0] = (uint8_t)4;
	buffer[1] = (uint8_t)(dictionary_size);
	buffer[2] = (uint8_t)(dictionary_size >> 8);
	buffer[3] = (uint8_t)(dictionary_size >> 16);
	buffer[4] = (uint8_t)(dictionary_size >> 24);

	return LZMA_RC_OK;
}


lzma_rc_t Lzma2Dec_AllocateProbs(lzma_2_decoder_t *p, uint8_t properties, lzma_allocator_ptr_t allocator)
{
	uint8_t properties[5];
	{ lzma_rc_t rc__ = (Lzma2Dec_GetOldProps(properties, properties)); if (rc__ != 0) return rc__; };
	return lzma_decoder_allocate_probabilities(&p->decoder, properties, 5, allocator);
}


lzma_rc_t Lzma2Dec_Allocate(lzma_2_decoder_t *p, uint8_t properties, lzma_allocator_ptr_t allocator)
{
	uint8_t properties[5];
	{ lzma_rc_t rc__ = (Lzma2Dec_GetOldProps(properties, properties)); if (rc__ != 0) return rc__; };
	return lzma_decoder_allocate(&p->decoder, properties, 5, allocator);
}


void Lzma2Dec_Init(lzma_2_decoder_t *p)
{
	p->state = LZMA2_STATE_CONTROL;
	p->need_to_init_dictionary = 1;
	p->need_to_init_state = 1;
	p->need_to_init_properties = 1;
	lzma_decoder_init(&p->decoder);
}


static lzma_2_state_t Lzma2Dec_UpdateState(lzma_2_decoder_t *p, uint8_t b)
{
	switch (p->state)
	{
	case LZMA2_STATE_CONTROL:

		p->control = b;

		if (b == 0) return LZMA2_STATE_FINISHED;

		if ((((p)->control & (1 << 7)) == 0))
		{
			if (b > 2) return LZMA2_STATE_ERROR;
			p->unpacked_size = 0;
		}
		else p->unpacked_size = (uint32_t)(b & 0x1F) << 16;

		return LZMA2_STATE_UNPACK0;

	case LZMA2_STATE_UNPACK0:

		p->unpacked_size |= (uint32_t)b << 8;
		return LZMA2_STATE_UNPACK1;

	case LZMA2_STATE_UNPACK1:
		p->unpacked_size |= (uint32_t)b;
		p->unpacked_size++;
		return ((((p)->control & (1 << 7)) == 0)) ? LZMA2_STATE_DATA : LZMA2_STATE_PACK0;

	case LZMA2_STATE_PACK0:
		p->packed_size = (uint32_t)b << 8;
		return LZMA2_STATE_PACK1;

	case LZMA2_STATE_PACK1:
		p->packed_size |= (uint32_t)b;
		p->packed_size++;
		return (((((p)->control >> 5) & 3)) >= 2) ? LZMA2_STATE_PROP :
			(p->need_to_init_properties ? LZMA2_STATE_ERROR : LZMA2_STATE_DATA);

	case LZMA2_STATE_PROP:
	{
		uint32_t lc, lp;
		if (b >= (9 * 5 * 5))
			return LZMA2_STATE_ERROR;
		lc = b % 9;
		b /= 9;
		p->decoder.properties.pb = b / 5;
		lp = b % 5;
		if (lc + lp > 4)
			return LZMA2_STATE_ERROR;
		p->decoder.properties.lc = lc;
		p->decoder.properties.lp = lp;
		p->need_to_init_properties = 0;
		return LZMA2_STATE_DATA;
	}
	}

	return LZMA2_STATE_ERROR;
}


static void LzmaDec_UpdateWithUncompressed(lzma_decoder_t *p, const uint8_t *src, size_t size)
{
	lzma_memcpy(p->dictionary + p->dictionary_position, src, size);

	p->dictionary_position += size;

	if (p->check_dictionary_size == 0 && p->properties.dictionary_size - p->processed_position <= size)
		p->check_dictionary_size = p->properties.dictionary_size;

	p->processed_position += (uint32_t)size;
}


void lzma_decoder_init_dictionary_and_state(lzma_decoder_t *p, uint8_t init_dictionary, uint8_t init_state);


lzma_rc_t Lzma2Dec_DecodeToDic(lzma_2_decoder_t *p, size_t dictionary_limit, const uint8_t *src, size_t *src_len, lzma_finish_mode_t finish_mode, lzma_status_t *status)
{
	size_t in_size = *src_len;
	*src_len = 0;
	*status = LZMA_STATUS_NOT_SPECIFIED;

	while (p->state != LZMA2_STATE_ERROR)
	{
		size_t dictionary_position;

		if (p->state == LZMA2_STATE_FINISHED)
		{
			*status = LZMA_STATUS_FINISHED_WITH_MARK;
			return LZMA_RC_OK;
		}

		dictionary_position = p->decoder.dictionary_position;

		if (dictionary_position == dictionary_limit && finish_mode == LZMA_FINISH_ANY)
		{
			*status = LZMA_STATUS_NOT_FINISHED;
			return LZMA_RC_OK;
		}

		if (p->state != LZMA2_STATE_DATA && p->state != LZMA2_STATE_DATA_CONT)
		{
			if (*src_len == in_size)
			{
				*status = LZMA_STATUS_NEEDS_MORE_INPUT;
				return LZMA_RC_OK;
			}

			(*src_len)++;
			p->state = Lzma2Dec_UpdateState(p, *src++);
			if (dictionary_position == dictionary_limit && p->state != LZMA2_STATE_FINISHED)
				break;

			continue;
		}

		{
			size_t inCur = in_size - *src_len;
			size_t outCur = dictionary_limit - dictionary_position;
			lzma_finish_mode_t curFinishMode = LZMA_FINISH_ANY;

			if (outCur >= p->unpacked_size)
			{
				outCur = (size_t)p->unpacked_size;
				curFinishMode = LZMA_FINISH_END;
			}

			if ((((p)->control & (1 << 7)) == 0))
			{
				if (inCur == 0)
				{
					*status = LZMA_STATUS_NEEDS_MORE_INPUT;
					return LZMA_RC_OK;
				}

				if (p->state == LZMA2_STATE_DATA)
				{
					uint8_t init_dictionary = (p->control == 1);
					if (init_dictionary)
						p->need_to_init_properties = p->need_to_init_state = 1;
					else if (p->need_to_init_dictionary)
						break;
					p->need_to_init_dictionary = 0;
					lzma_decoder_init_dictionary_and_state(&p->decoder, init_dictionary, 0);
				}

				if (inCur > outCur) inCur = outCur;
				if (inCur == 0) break;

				LzmaDec_UpdateWithUncompressed(&p->decoder, src, inCur);

				src += inCur;
				*src_len += inCur;
				p->unpacked_size -= (uint32_t)inCur;
				p->state = (p->unpacked_size == 0) ? LZMA2_STATE_CONTROL : LZMA2_STATE_DATA_CONT;
			}
			else
			{
				lzma_rc_t res;

				if (p->state == LZMA2_STATE_DATA)
				{
					uint32_t mode = (((p)->control >> 5) & 3);
					uint8_t init_dictionary = (mode == 3);
					uint8_t init_state = (mode != 0);
					if ((!init_dictionary && p->need_to_init_dictionary) || (!init_state && p->need_to_init_state))
						break;

					lzma_decoder_init_dictionary_and_state(&p->decoder, init_dictionary, init_state);
					p->need_to_init_dictionary = 0;
					p->need_to_init_state = 0;
					p->state = LZMA2_STATE_DATA_CONT;
				}

				if (inCur > p->packed_size)
					inCur = (size_t)p->packed_size;

				res = lzma_decoder_decode_to_dictionary(&p->decoder, dictionary_position + outCur, src, &inCur, curFinishMode, status);

				src += inCur;
				*src_len += inCur;
				p->packed_size -= (uint32_t)inCur;
				outCur = p->decoder.dictionary_position - dictionary_position;
				p->unpacked_size -= (uint32_t)outCur;

				if (res != 0)
					break;

				if (*status == LZMA_STATUS_NEEDS_MORE_INPUT)
				{
					if (p->packed_size == 0)
						break;
					return LZMA_RC_OK;
				}

				if (inCur == 0 && outCur == 0)
				{
					if (*status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK || p->unpacked_size != 0 || p->packed_size != 0)
						break;

					p->state = LZMA2_STATE_CONTROL;
				}

				*status = LZMA_STATUS_NOT_SPECIFIED;
			}
		}
	}

	*status = LZMA_STATUS_NOT_SPECIFIED;
	p->state = LZMA2_STATE_ERROR;

	return LZMA_RC_DATA;
}


lzma_rc_t Lzma2Dec_DecodeToBuf(lzma_2_decoder_t *p, uint8_t *dest, size_t *dest_len, const uint8_t *src, size_t *src_len, lzma_finish_mode_t finish_mode, lzma_status_t *status)
{
	size_t out_size = *dest_len, in_size = *src_len;
	*src_len = *dest_len = 0;

	for (;;)
	{
		size_t inCur = in_size, outCur, dictionary_position;
		lzma_finish_mode_t curFinishMode;
		lzma_rc_t res;

		if (p->decoder.dictionary_position == p->decoder.dictionary_buffer_size)
			p->decoder.dictionary_position = 0;

		dictionary_position = p->decoder.dictionary_position;
		curFinishMode = LZMA_FINISH_ANY;
		outCur = p->decoder.dictionary_buffer_size - dictionary_position;

		if (outCur >= out_size)
		{
			outCur = out_size;
			curFinishMode = finish_mode;
		}

		res = Lzma2Dec_DecodeToDic(p, dictionary_position + outCur, src, &inCur, curFinishMode, status);

		src += inCur;
		in_size -= inCur;
		*src_len += inCur;
		outCur = p->decoder.dictionary_position - dictionary_position;
		lzma_memcpy(dest, p->decoder.dictionary + dictionary_position, outCur);
		dest += outCur;
		out_size -= outCur;
		*dest_len += outCur;

		if (res != 0) return res;
		if (outCur == 0 || out_size == 0) return LZMA_RC_OK;
	}
}


lzma_rc_t Lzma2Decode(uint8_t *dest, size_t *dest_len, const uint8_t *src, size_t *src_len, uint8_t properties, lzma_finish_mode_t finish_mode, lzma_status_t *status, lzma_allocator_ptr_t allocator)
{
	lzma_2_decoder_t p;
	lzma_rc_t res;
	size_t out_size = *dest_len, in_size = *src_len;

	*dest_len = *src_len = 0;
	*status = LZMA_STATUS_NOT_SPECIFIED;

	{ (&(&p)->decoder)->dictionary = 0; (&(&p)->decoder)->probabilities = 0; };
	{ lzma_rc_t rc__ = (Lzma2Dec_AllocateProbs(&p, properties, allocator)); if (rc__ != 0) return rc__; };

	p.decoder.dictionary = dest;
	p.decoder.dictionary_buffer_size = out_size;

	Lzma2Dec_Init(&p);

	*src_len = in_size;

	res = Lzma2Dec_DecodeToDic(&p, out_size, src, src_len, finish_mode, status);

	*dest_len = p.decoder.dictionary_position;

	if (res == LZMA_RC_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT)
		res = LZMA_RC_INPUT_EOF;

	lzma_decoder_release_probabilities(&(&p)->decoder, allocator);

	return res;
}


typedef uint32_t CLzRef;


typedef struct lzma_match_finder_s
{
	uint8_t* buffer;
	uint32_t position;
	uint32_t posLimit;
	uint32_t streamPos;
	uint32_t lenLimit;
	uint32_t cyclicBufferPos;
	uint32_t cyclicBufferSize;
	uint8_t streamEndWasReached;
	uint8_t mode_bt;
	uint8_t bigHash;
	uint8_t directInput;
	uint32_t matchMaxLen;
	CLzRef* hash;
	CLzRef* son;
	uint32_t hashMask;
	uint32_t cutValue;
	uint8_t* bufferBase;
	lzma_seq_in_stream_t* stream;
	uint32_t block_size;
	uint32_t keepSizeBefore;
	uint32_t keepSizeAfter;
	uint32_t hash_byte_count;
	size_t directInputRem;
	uint32_t historySize;
	uint32_t fixedHashSize;
	uint32_t hashSizeSum;
	lzma_rc_t result;
	uint32_t crc[256];
	size_t numRefs;
	uint64_t expectedDataSize;
}
lzma_match_finder_t;


int32_t MatchFinder_NeedMove(lzma_match_finder_t *p);
uint8_t *MatchFinder_GetPointerToCurrentPos(lzma_match_finder_t *p);
void MatchFinder_MoveBlock(lzma_match_finder_t *p);
void MatchFinder_ReadIfRequired(lzma_match_finder_t *p);
void MatchFinder_Construct(lzma_match_finder_t *p);
int32_t MatchFinder_Create(lzma_match_finder_t *p, uint32_t historySize, uint32_t keepAddBufferBefore, uint32_t matchMaxLen, uint32_t keepAddBufferAfter, lzma_allocator_ptr_t allocator);
void MatchFinder_Free(lzma_match_finder_t *p, lzma_allocator_ptr_t allocator);
void MatchFinder_Normalize3(uint32_t subValue, CLzRef *items, size_t numItems);
void MatchFinder_ReduceOffsets(lzma_match_finder_t *p, uint32_t subValue);
uint32_t* GetMatchesSpec1(uint32_t lenLimit, uint32_t curMatch, uint32_t pos, const uint8_t *buffer, CLzRef *son, uint32_t _cyclicBufferPos, uint32_t _cyclicBufferSize, uint32_t _cutValue, uint32_t *distances, uint32_t maxLen);


typedef void (*Mf_Init_Func)(void* object);
typedef uint32_t (*Mf_GetNumAvailableBytes_Func)(void* object);
typedef const uint8_t* (*Mf_GetPointerToCurrentPos_Func)(void* object);
typedef uint32_t (*Mf_GetMatches_Func)(void* object, uint32_t* distances);
typedef void (*Mf_Skip_Func)(void* object, uint32_t);


typedef struct _IMatchFinder
{
	Mf_Init_Func Init;
	Mf_GetNumAvailableBytes_Func GetNumAvailableBytes;
	Mf_GetPointerToCurrentPos_Func GetPointerToCurrentPos;
	Mf_GetMatches_Func GetMatches;
	Mf_Skip_Func skip;
}
IMatchFinder;


void MatchFinder_CreateVTable(lzma_match_finder_t *p, IMatchFinder *vTable);
void MatchFinder_Init_LowHash(lzma_match_finder_t *p);
void MatchFinder_Init_HighHash(lzma_match_finder_t *p);
void MatchFinder_Init_3(lzma_match_finder_t *p, int32_t readData);
void MatchFinder_Init(lzma_match_finder_t *p);
uint32_t Bt3Zip_MatchFinder_GetMatches(lzma_match_finder_t *p, uint32_t *distances);
uint32_t Hc3Zip_MatchFinder_GetMatches(lzma_match_finder_t *p, uint32_t *distances);
void Bt3Zip_MatchFinder_Skip(lzma_match_finder_t *p, uint32_t num);
void Hc3Zip_MatchFinder_Skip(lzma_match_finder_t *p, uint32_t num);


static void LzInWindow_Free(lzma_match_finder_t *p, lzma_allocator_ptr_t allocator)
{
	if (!p->directInput)
	{
		(allocator)->release(allocator, p->bufferBase);
		p->bufferBase = ((void *)0);
	}
}


static int32_t LzInWindow_Create(lzma_match_finder_t *p, uint32_t keepSizeReserv, lzma_allocator_ptr_t allocator)
{
	uint32_t block_size = p->keepSizeBefore + p->keepSizeAfter + keepSizeReserv;

	if (p->directInput)
	{
		p->block_size = block_size;
		return 1;
	}

	if (!p->bufferBase || p->block_size != block_size)
	{
		LzInWindow_Free(p, allocator);

		p->block_size = block_size;
		p->bufferBase = (uint8_t *)(allocator)->allocate(allocator, (size_t)block_size);
	}

	return (p->bufferBase != ((void *)0));
}


uint8_t* MatchFinder_GetPointerToCurrentPos(lzma_match_finder_t *p) { return p->buffer; }
uint32_t MatchFinder_GetNumAvailableBytes(lzma_match_finder_t *p) { return p->streamPos - p->position; }


void MatchFinder_ReduceOffsets(lzma_match_finder_t *p, uint32_t subValue)
{
	p->posLimit -= subValue;
	p->position -= subValue;
	p->streamPos -= subValue;
}


static void MatchFinder_ReadBlock(lzma_match_finder_t *p)
{
	if (p->streamEndWasReached || p->result != LZMA_RC_OK)
		return;

	if (p->directInput)
	{
		uint32_t curSize = 0xFFFFFFFF - (p->streamPos - p->position);

		if (curSize > p->directInputRem)
			curSize = (uint32_t)p->directInputRem;

		p->directInputRem -= curSize;
		p->streamPos += curSize;

		if (p->directInputRem == 0)
			p->streamEndWasReached = 1;

		return;
	}

	for (;;)
	{
		uint8_t *dest = p->buffer + (p->streamPos - p->position);
		size_t size = (p->bufferBase + p->block_size - dest);

		if (size == 0)
			return;

		p->result = (p->stream)->read(p->stream, dest, &size);

		if (p->result != LZMA_RC_OK)
			return;

		if (size == 0)
		{
			p->streamEndWasReached = 1;
			return;
		}

		p->streamPos += (uint32_t)size;

		if (p->streamPos - p->position > p->keepSizeAfter)
			return;
	}
}


void MatchFinder_MoveBlock(lzma_match_finder_t *p)
{
	lzma_memmove(p->bufferBase, p->buffer - p->keepSizeBefore, (size_t)(p->streamPos - p->position) + p->keepSizeBefore);
	p->buffer = p->bufferBase + p->keepSizeBefore;
}


int32_t MatchFinder_NeedMove(lzma_match_finder_t *p)
{
	if (p->directInput) return 0;
	return ((size_t)(p->bufferBase + p->block_size - p->buffer) <= p->keepSizeAfter);
}


void MatchFinder_ReadIfRequired(lzma_match_finder_t *p)
{
	if (p->streamEndWasReached)
		return;

	if (p->keepSizeAfter >= p->streamPos - p->position)
		MatchFinder_ReadBlock(p);
}


static void MatchFinder_CheckAndMoveAndRead(lzma_match_finder_t *p)
{
	if (MatchFinder_NeedMove(p))
		MatchFinder_MoveBlock(p);

	MatchFinder_ReadBlock(p);
}


static void MatchFinder_SetDefaultSettings(lzma_match_finder_t *p)
{
	p->cutValue = 32;
	p->mode_bt = 1;
	p->hash_byte_count = 4;
	p->bigHash = 0;
}





void MatchFinder_Construct(lzma_match_finder_t *p)
{
	uint32_t i;

	p->bufferBase = ((void *)0);
	p->directInput = 0;
	p->hash = ((void *)0);
	p->expectedDataSize = (uint64_t)(int64_t)-1;
	MatchFinder_SetDefaultSettings(p);

	for (i = 0; i < 256; ++i)
	{
		uint32_t r = i;
		uint32_t j;

		for (j = 0; j < 8; j++)
			r = (r >> 1) ^ (0xEDB88320 & ((uint32_t)0 - (r & 1)));

		p->crc[i] = r;
	}
}


static void MatchFinder_FreeThisClassMemory(lzma_match_finder_t *p, lzma_allocator_ptr_t allocator)
{
	(allocator)->release(allocator, p->hash);
	p->hash = ((void *)0);
}


void MatchFinder_Free(lzma_match_finder_t *p, lzma_allocator_ptr_t allocator)
{
	MatchFinder_FreeThisClassMemory(p, allocator);
	LzInWindow_Free(p, allocator);
}


static CLzRef* AllocRefs(size_t num, lzma_allocator_ptr_t allocator)
{
	size_t sizeInBytes = (size_t)num * sizeof(CLzRef);

	if (sizeInBytes / sizeof(CLzRef) != num)
		return ((void *)0);

	return (CLzRef *)(allocator)->allocate(allocator, sizeInBytes);
}


int32_t MatchFinder_Create(lzma_match_finder_t *p, uint32_t historySize, uint32_t keepAddBufferBefore, uint32_t matchMaxLen, uint32_t keepAddBufferAfter, lzma_allocator_ptr_t allocator)
{
	uint32_t sizeReserv;

	if (historySize > ((uint32_t)7 << 29))
	{
		MatchFinder_Free(p, allocator);
		return 0;
	}

	sizeReserv = historySize >> 1;
	if (historySize >= ((uint32_t)3 << 30)) sizeReserv = historySize >> 3;
	else if (historySize >= ((uint32_t)2 << 30)) sizeReserv = historySize >> 2;

	sizeReserv += (keepAddBufferBefore + matchMaxLen + keepAddBufferAfter) / 2 + (1 << 19);

	p->keepSizeBefore = historySize + keepAddBufferBefore + 1;
	p->keepSizeAfter = matchMaxLen + keepAddBufferAfter;

	if (LzInWindow_Create(p, sizeReserv, allocator))
	{
		uint32_t newCyclicBufferSize = historySize + 1;
		uint32_t hs;

		p->matchMaxLen = matchMaxLen;
		{
			p->fixedHashSize = 0;

			if (p->hash_byte_count == 2)
				hs = (1 << 16) - 1;
			else
			{
				hs = historySize;

				if (hs > p->expectedDataSize)
					hs = (uint32_t)p->expectedDataSize;

				if (hs != 0)
					hs--;

				hs |= (hs >> 1);
				hs |= (hs >> 2);
				hs |= (hs >> 4);
				hs |= (hs >> 8);
				hs >>= 1;
				hs |= 0xFFFF;

				if (hs > (1 << 24))
				{
					if (p->hash_byte_count == 3)
						hs = (1 << 24) - 1;
					else hs >>= 1;
				}
			}

			p->hashMask = hs;
			hs++;

			if (p->hash_byte_count > 2) p->fixedHashSize += (1 << 10);
			if (p->hash_byte_count > 3) p->fixedHashSize += (1 << 16);
			if (p->hash_byte_count > 4) p->fixedHashSize += (1 << 20);

			hs += p->fixedHashSize;
		}

		{
			size_t newSize;
			size_t numSons;
			p->historySize = historySize;
			p->hashSizeSum = hs;
			p->cyclicBufferSize = newCyclicBufferSize;

			numSons = newCyclicBufferSize;

			if (p->mode_bt)
				numSons <<= 1;

			newSize = hs + numSons;

			if (p->hash && p->numRefs == newSize)
				return 1;

			MatchFinder_FreeThisClassMemory(p, allocator);
			p->numRefs = newSize;
			p->hash = AllocRefs(newSize, allocator);

			if (p->hash)
			{
				p->son = p->hash + p->hashSizeSum;

				return 1;
			}
		}
	}

	MatchFinder_Free(p, allocator);

	return 0;
}


static void MatchFinder_SetLimits(lzma_match_finder_t *p)
{
	uint32_t limit = (UINT32_C(0xFFFFFFFF)) - p->position;
	uint32_t limit_b = p->cyclicBufferSize - p->cyclicBufferPos;

	if (limit_b < limit)
		limit = limit_b;

	limit_b = p->streamPos - p->position;

	if (limit_b <= p->keepSizeAfter)
	{
		if (limit_b > 0)
			limit_b = 1;
	}
	else limit_b -= p->keepSizeAfter;

	if (limit_b < limit)
		limit = limit_b;

	{
		uint32_t lenLimit = p->streamPos - p->position;

		if (lenLimit > p->matchMaxLen)
			lenLimit = p->matchMaxLen;

		p->lenLimit = lenLimit;
	}

	p->posLimit = p->position + limit;
}


void MatchFinder_Init_LowHash(lzma_match_finder_t *p)
{
	size_t i;
	CLzRef *items = p->hash;
	size_t numItems = p->fixedHashSize;
	for (i = 0; i < numItems; ++i)
		items[i] = 0;
}


void MatchFinder_Init_HighHash(lzma_match_finder_t *p)
{
	size_t i;
	CLzRef *items = p->hash + p->fixedHashSize;
	size_t numItems = (size_t)p->hashMask + 1;

	for (i = 0; i < numItems; ++i)
		items[i] = 0;
}


void MatchFinder_Init_3(lzma_match_finder_t *p, int32_t readData)
{
	p->cyclicBufferPos = 0;
	p->buffer = p->bufferBase;
	p->position =
		p->streamPos = p->cyclicBufferSize;
	p->result = LZMA_RC_OK;
	p->streamEndWasReached = 0;

	if (readData)
		MatchFinder_ReadBlock(p);

	MatchFinder_SetLimits(p);
}


void MatchFinder_Init(lzma_match_finder_t *p)
{
	MatchFinder_Init_HighHash(p);
	MatchFinder_Init_LowHash(p);
	MatchFinder_Init_3(p, 1);
}


static uint32_t MatchFinder_GetSubValue(lzma_match_finder_t *p)
{
	return (p->position - p->historySize - 1) & (~(uint32_t)((1 << 10) - 1));
}


void MatchFinder_Normalize3(uint32_t subValue, CLzRef *items, size_t numItems)
{
	size_t i;

	for (i = 0; i < numItems; ++i)
	{
		uint32_t value = items[i];

		if (value <= subValue)
			value = 0;
		else value -= subValue;

		items[i] = value;
	}
}


static void MatchFinder_Normalize(lzma_match_finder_t *p)
{
	uint32_t subValue = MatchFinder_GetSubValue(p);
	MatchFinder_Normalize3(subValue, p->hash, p->numRefs);
	MatchFinder_ReduceOffsets(p, subValue);
}


static void MatchFinder_CheckLimits(lzma_match_finder_t *p)
{
	if (p->position == (UINT32_C(0xFFFFFFFF)))
		MatchFinder_Normalize(p);

	if (!p->streamEndWasReached && p->keepSizeAfter == p->streamPos - p->position)
		MatchFinder_CheckAndMoveAndRead(p);

	if (p->cyclicBufferPos == p->cyclicBufferSize)
		p->cyclicBufferPos = 0;

	MatchFinder_SetLimits(p);
}


static uint32_t * Hc_GetMatchesSpec(uint32_t lenLimit, uint32_t curMatch, uint32_t pos, const uint8_t *cur, CLzRef *son, uint32_t _cyclicBufferPos, uint32_t _cyclicBufferSize, uint32_t cutValue, uint32_t *distances, uint32_t maxLen)
{
	son[_cyclicBufferPos] = curMatch;

	for (;;)
	{
		uint32_t delta = pos - curMatch;

		if (cutValue-- == 0 || delta >= _cyclicBufferSize)
			return distances;

		{
			const uint8_t *pb = cur - delta;
			curMatch = son[_cyclicBufferPos - delta + ((delta > _cyclicBufferPos) ? _cyclicBufferSize : 0)];

			if (pb[maxLen] == cur[maxLen] && *pb == *cur)
			{
				uint32_t len = 0;
				while (++len != lenLimit)
					if (pb[len] != cur[len])
						break;

				if (maxLen < len)
				{
					*distances++ = maxLen = len;
					*distances++ = delta - 1;

					if (len == lenLimit)
						return distances;
				}
			}
		}
	}
}


uint32_t * GetMatchesSpec1(uint32_t lenLimit, uint32_t curMatch, uint32_t pos, const uint8_t *cur, CLzRef *son, uint32_t _cyclicBufferPos, uint32_t _cyclicBufferSize, uint32_t cutValue, uint32_t *distances, uint32_t maxLen)
{
	CLzRef *ptr0 = son + (_cyclicBufferPos << 1) + 1;
	CLzRef *ptr1 = son + (_cyclicBufferPos << 1);
	uint32_t len0 = 0, len1 = 0;

	for (;;)
	{
		uint32_t delta = pos - curMatch;
		if (cutValue-- == 0 || delta >= _cyclicBufferSize)
		{
			*ptr0 = *ptr1 = 0;
			return distances;
		}

		{
			CLzRef *pair = son + ((_cyclicBufferPos - delta + ((delta > _cyclicBufferPos) ? _cyclicBufferSize : 0)) << 1);
			const uint8_t *pb = cur - delta;
			uint32_t len = (len0 < len1 ? len0 : len1);

			if (pb[len] == cur[len])
			{
				if (++len != lenLimit && pb[len] == cur[len])
					while (++len != lenLimit)
						if (pb[len] != cur[len])
							break;

				if (maxLen < len)
				{
					*distances++ = maxLen = len;
					*distances++ = delta - 1;

					if (len == lenLimit)
					{
						*ptr1 = pair[0];
						*ptr0 = pair[1];

						return distances;
					}
				}
			}

			if (pb[len] < cur[len])
			{
				*ptr1 = curMatch;
				ptr1 = pair + 1;
				curMatch = *ptr1;
				len1 = len;
			}
			else
			{
				*ptr0 = curMatch;
				ptr0 = pair;
				curMatch = *ptr0;
				len0 = len;
			}
		}
	}
}


static void SkipMatchesSpec(uint32_t lenLimit, uint32_t curMatch, uint32_t pos, const uint8_t *cur, CLzRef *son, uint32_t _cyclicBufferPos, uint32_t _cyclicBufferSize, uint32_t cutValue)
{
	CLzRef *ptr0 = son + (_cyclicBufferPos << 1) + 1;
	CLzRef *ptr1 = son + (_cyclicBufferPos << 1);
	uint32_t len0 = 0, len1 = 0;

	for (;;)
	{
		uint32_t delta = pos - curMatch;

		if (cutValue-- == 0 || delta >= _cyclicBufferSize)
		{
			*ptr0 = *ptr1 = 0;
			return;
		}

		{
			CLzRef *pair = son + ((_cyclicBufferPos - delta + ((delta > _cyclicBufferPos) ? _cyclicBufferSize : 0)) << 1);
			const uint8_t *pb = cur - delta;
			uint32_t len = (len0 < len1 ? len0 : len1);

			if (pb[len] == cur[len])
			{
				while (++len != lenLimit)
					if (pb[len] != cur[len])
						break;

				{
					if (len == lenLimit)
					{
						*ptr1 = pair[0];
						*ptr0 = pair[1];
						return;
					}
				}
			}

			if (pb[len] < cur[len])
			{
				*ptr1 = curMatch;
				ptr1 = pair + 1;
				curMatch = *ptr1;
				len1 = len;
			}
			else
			{
				*ptr0 = curMatch;
				ptr0 = pair;
				curMatch = *ptr0;
				len0 = len;
			}
		}
	}
}






static void MatchFinder_MovePos(lzma_match_finder_t *p) { ++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p); }











static uint32_t Bt2_MatchFinder_GetMatches(lzma_match_finder_t *p, uint32_t *distances)
{
	uint32_t offset;

	uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 2) { MatchFinder_MovePos(p); return 0; } } cur = p->buffer;

	hv = cur[0] | ((uint32_t)cur[1] << 8);

	curMatch = p->hash[hv];
	p->hash[hv] = p->position;
	offset = 0;
	offset = (uint32_t)(GetMatchesSpec1(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue, distances + offset, 1) - distances); ++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p); return offset;
}


uint32_t Bt3Zip_MatchFinder_GetMatches(lzma_match_finder_t *p, uint32_t *distances)
{
	uint32_t offset;

	uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 3) { MatchFinder_MovePos(p); return 0; } } cur = p->buffer;

	hv = ((cur[2] | ((uint32_t)cur[0] << 8)) ^ p->crc[cur[1]]) & 0xFFFF;

	curMatch = p->hash[hv];
	p->hash[hv] = p->position;
	offset = 0;

	offset = (uint32_t)(GetMatchesSpec1(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue, distances + offset, 2) - distances); ++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p); return offset;
}

static uint32_t Bt3_MatchFinder_GetMatches(lzma_match_finder_t *p, uint32_t *distances)
{
	uint32_t h2, d2, maxLen, offset, pos;
	uint32_t *hash;

	uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 3) { MatchFinder_MovePos(p); return 0; } } cur = p->buffer;

	{ uint32_t temp = p->crc[cur[0]] ^ cur[1]; h2 = temp & ((1 << 10) - 1); hv = (temp ^ ((uint32_t)cur[2] << 8)) & p->hashMask; };

	hash = p->hash;
	pos = p->position;

	d2 = pos - hash[h2];

	curMatch = (hash + ((1 << 10)))[hv];

	hash[h2] = pos;
	(hash + ((1 << 10)))[hv] = pos;

	maxLen = 2;
	offset = 0;

	if (d2 < p->cyclicBufferSize && *(cur - d2) == *cur)
	{
		{ ptrdiff_t diff = (ptrdiff_t)0 - d2; const uint8_t *c = cur + maxLen; const uint8_t *lim = cur + lenLimit; for (; c != lim; c++) if (*(c + diff) != *c) break; maxLen = (uint32_t)(c - cur); };

		distances[0] = maxLen;
		distances[1] = d2 - 1;
		offset = 2;

		if (maxLen == lenLimit)
		{
			SkipMatchesSpec(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue);
			++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p); return offset;
		}
	}

	offset = (uint32_t)(GetMatchesSpec1(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue, distances + offset, maxLen) - distances); ++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p); return offset;
}


static uint32_t Bt4_MatchFinder_GetMatches(lzma_match_finder_t *p, uint32_t *distances)
{
	uint32_t h2, h3, d2, d3, maxLen, offset, pos;
	uint32_t *hash;

	uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 4) { MatchFinder_MovePos(p); return 0; } } cur = p->buffer;

	{ uint32_t temp = p->crc[cur[0]] ^ cur[1]; h2 = temp & ((1 << 10) - 1); temp ^= ((uint32_t)cur[2] << 8); h3 = temp & ((1 << 16) - 1); hv = (temp ^ (p->crc[cur[3]] << 5)) & p->hashMask; };

	hash = p->hash;
	pos = p->position;

	d2 = pos - hash[h2];
	d3 = pos - (hash + ((1 << 10)))[h3];

	curMatch = (hash + ((1 << 10) + (1 << 16)))[hv];

	hash[h2] = pos;
	(hash + ((1 << 10)))[h3] = pos;
	(hash + ((1 << 10) + (1 << 16)))[hv] = pos;

	maxLen = 0;
	offset = 0;

	if (d2 < p->cyclicBufferSize && *(cur - d2) == *cur)
	{
		distances[0] = maxLen = 2;
		distances[1] = d2 - 1;
		offset = 2;
	}

	if (d2 != d3 && d3 < p->cyclicBufferSize && *(cur - d3) == *cur)
	{
		maxLen = 3;
		distances[(size_t)offset + 1] = d3 - 1;
		offset += 2;
		d2 = d3;
	}

	if (offset != 0)
	{
		{ ptrdiff_t diff = (ptrdiff_t)0 - d2; const uint8_t *c = cur + maxLen; const uint8_t *lim = cur + lenLimit; for (; c != lim; c++) if (*(c + diff) != *c) break; maxLen = (uint32_t)(c - cur); };
		distances[(size_t)offset - 2] = maxLen;

		if (maxLen == lenLimit)
		{
			SkipMatchesSpec(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue);
			++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p); return offset;
		}
	}

	if (maxLen < 3)
		maxLen = 3;

	offset = (uint32_t)(GetMatchesSpec1(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue, distances + offset, maxLen) - distances); ++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p); return offset;
}


static uint32_t Hc4_MatchFinder_GetMatches(lzma_match_finder_t *p, uint32_t *distances)
{
	uint32_t h2, h3, d2, d3, maxLen, offset, pos;
	uint32_t *hash;

	uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 4) { MatchFinder_MovePos(p); return 0; } } cur = p->buffer;

	{ uint32_t temp = p->crc[cur[0]] ^ cur[1]; h2 = temp & ((1 << 10) - 1); temp ^= ((uint32_t)cur[2] << 8); h3 = temp & ((1 << 16) - 1); hv = (temp ^ (p->crc[cur[3]] << 5)) & p->hashMask; };

	hash = p->hash;
	pos = p->position;

	d2 = pos - hash[h2];
	d3 = pos - (hash + ((1 << 10)))[h3];

	curMatch = (hash + ((1 << 10) + (1 << 16)))[hv];

	hash[h2] = pos;
	(hash + ((1 << 10)))[h3] = pos;
	(hash + ((1 << 10) + (1 << 16)))[hv] = pos;

	maxLen = 0;
	offset = 0;

	if (d2 < p->cyclicBufferSize && *(cur - d2) == *cur)
	{
		distances[0] = maxLen = 2;
		distances[1] = d2 - 1;
		offset = 2;
	}

	if (d2 != d3 && d3 < p->cyclicBufferSize && *(cur - d3) == *cur)
	{
		maxLen = 3;
		distances[(size_t)offset + 1] = d3 - 1;
		offset += 2;
		d2 = d3;
	}

	if (offset != 0)
	{
		{ ptrdiff_t diff = (ptrdiff_t)0 - d2; const uint8_t *c = cur + maxLen; const uint8_t *lim = cur + lenLimit; for (; c != lim; c++) if (*(c + diff) != *c) break; maxLen = (uint32_t)(c - cur); };
		distances[(size_t)offset - 2] = maxLen;

		if (maxLen == lenLimit)
		{
			p->son[p->cyclicBufferPos] = curMatch;
			++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p); return offset;
		}
	}

	if (maxLen < 3) maxLen = 3;

	offset = (uint32_t)(Hc_GetMatchesSpec(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue, distances + offset, maxLen) - (distances));

	++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p); return offset;
}


uint32_t Hc3Zip_MatchFinder_GetMatches(lzma_match_finder_t *p, uint32_t *distances)
{
	uint32_t offset;
	uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 3) { MatchFinder_MovePos(p); return 0; } } cur = p->buffer;
	hv = ((cur[2] | ((uint32_t)cur[0] << 8)) ^ p->crc[cur[1]]) & 0xFFFF;
	curMatch = p->hash[hv];
	p->hash[hv] = p->position;
	offset = (uint32_t)(Hc_GetMatchesSpec(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue, distances, 2) - (distances));
	++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p); return offset;
}


static void Bt2_MatchFinder_Skip(lzma_match_finder_t *p, uint32_t num)
{
	do
	{
		uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 2) { MatchFinder_MovePos(p); continue; } } cur = p->buffer;
		hv = cur[0] | ((uint32_t)cur[1] << 8);
		curMatch = p->hash[hv];
		p->hash[hv] = p->position;
		SkipMatchesSpec(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue); ++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p);
	} while (--num != 0);
}


void Bt3Zip_MatchFinder_Skip(lzma_match_finder_t *p, uint32_t num)
{
	do
	{
		uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 3) { MatchFinder_MovePos(p); continue; } } cur = p->buffer;
		hv = ((cur[2] | ((uint32_t)cur[0] << 8)) ^ p->crc[cur[1]]) & 0xFFFF;
		curMatch = p->hash[hv];
		p->hash[hv] = p->position;
		SkipMatchesSpec(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue); ++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p);
	} while (--num != 0);
}


static void Bt3_MatchFinder_Skip(lzma_match_finder_t *p, uint32_t num)
{
	do
	{
		uint32_t h2;
		uint32_t *hash;
		uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 3) { MatchFinder_MovePos(p); continue; } } cur = p->buffer;
		{ uint32_t temp = p->crc[cur[0]] ^ cur[1]; h2 = temp & ((1 << 10) - 1); hv = (temp ^ ((uint32_t)cur[2] << 8)) & p->hashMask; };
		hash = p->hash;
		curMatch = (hash + ((1 << 10)))[hv];
		hash[h2] = (hash + ((1 << 10)))[hv] = p->position;
		SkipMatchesSpec(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue); ++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p);
	} while (--num != 0);
}


static void Bt4_MatchFinder_Skip(lzma_match_finder_t *p, uint32_t num)
{
	do
	{
		uint32_t h2, h3;
		uint32_t *hash;
		uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 4) { MatchFinder_MovePos(p); continue; } } cur = p->buffer;
		{ uint32_t temp = p->crc[cur[0]] ^ cur[1]; h2 = temp & ((1 << 10) - 1); temp ^= ((uint32_t)cur[2] << 8); h3 = temp & ((1 << 16) - 1); hv = (temp ^ (p->crc[cur[3]] << 5)) & p->hashMask; };
		hash = p->hash;
		curMatch = (hash + ((1 << 10) + (1 << 16)))[hv];
		hash[h2] = (hash + ((1 << 10)))[h3] = (hash + ((1 << 10) + (1 << 16)))[hv] = p->position;
		SkipMatchesSpec(lenLimit, curMatch, p->position, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue); ++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p);
	} while (--num != 0);
}


static void Hc4_MatchFinder_Skip(lzma_match_finder_t *p, uint32_t num)
{
	do
	{
		uint32_t h2, h3;
		uint32_t *hash;
		uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 4) { MatchFinder_MovePos(p); continue; } } cur = p->buffer;
		{ uint32_t temp = p->crc[cur[0]] ^ cur[1]; h2 = temp & ((1 << 10) - 1); temp ^= ((uint32_t)cur[2] << 8); h3 = temp & ((1 << 16) - 1); hv = (temp ^ (p->crc[cur[3]] << 5)) & p->hashMask; };
		hash = p->hash;
		curMatch = (hash + ((1 << 10) + (1 << 16)))[hv];
		hash[h2] = (hash + ((1 << 10)))[h3] = (hash + ((1 << 10) + (1 << 16)))[hv] = p->position;
		p->son[p->cyclicBufferPos] = curMatch;
		++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p);
	} while (--num != 0);
}


void Hc3Zip_MatchFinder_Skip(lzma_match_finder_t *p, uint32_t num)
{
	do
	{
		uint32_t lenLimit; uint32_t hv; const uint8_t *cur; uint32_t curMatch; lenLimit = p->lenLimit; { if (lenLimit < 3) { MatchFinder_MovePos(p); continue; } } cur = p->buffer;
		hv = ((cur[2] | ((uint32_t)cur[0] << 8)) ^ p->crc[cur[1]]) & 0xFFFF;
		curMatch = p->hash[hv];
		p->hash[hv] = p->position;
		p->son[p->cyclicBufferPos] = curMatch;
		++p->cyclicBufferPos; p->buffer++; if (++p->position == p->posLimit) MatchFinder_CheckLimits(p);
	} while (--num != 0);
}

void MatchFinder_CreateVTable(lzma_match_finder_t *p, IMatchFinder *vTable)
{
	vTable->Init = (Mf_Init_Func)MatchFinder_Init;
	vTable->GetNumAvailableBytes = (Mf_GetNumAvailableBytes_Func)MatchFinder_GetNumAvailableBytes;
	vTable->GetPointerToCurrentPos = (Mf_GetPointerToCurrentPos_Func)MatchFinder_GetPointerToCurrentPos;

	if (!p->mode_bt)
	{
		vTable->GetMatches = (Mf_GetMatches_Func)Hc4_MatchFinder_GetMatches;
		vTable->skip = (Mf_Skip_Func)Hc4_MatchFinder_Skip;
	}
	else if (p->hash_byte_count == 2)
	{
		vTable->GetMatches = (Mf_GetMatches_Func)Bt2_MatchFinder_GetMatches;
		vTable->skip = (Mf_Skip_Func)Bt2_MatchFinder_Skip;
	}
	else if (p->hash_byte_count == 3)
	{
		vTable->GetMatches = (Mf_GetMatches_Func)Bt3_MatchFinder_GetMatches;
		vTable->skip = (Mf_Skip_Func)Bt3_MatchFinder_Skip;
	}
	else
	{
		vTable->GetMatches = (Mf_GetMatches_Func)Bt4_MatchFinder_GetMatches;
		vTable->skip = (Mf_Skip_Func)Bt4_MatchFinder_Skip;
	}
}


void lzma_encoder_properties_init(lzma_encoder_properties_t *p)
{
	p->level = 5;
	p->dictionary_size = p->mc = 0;
	p->reduce_size = (uint64_t)(int64_t)-1;
	p->lc = p->lp = p->pb = p->algorithm = p->fb = p->mode_bt = p->hash_byte_count = p->thread_count = -1;
	p->write_end_mark = 0;
}

void lzma_encoder_properties_normalize(lzma_encoder_properties_t *p)
{
	int32_t level = p->level;
	if (level < 0) level = 5;
	p->level = level;

	if (p->dictionary_size == 0) p->dictionary_size = (level <= 5 ? (1 << (level * 2 + 14)) : (level <= 7 ? (1 << 25) : (1 << 26)));
	if (p->dictionary_size > p->reduce_size)
	{
		uint32_t i;
		uint32_t reduce_size = (uint32_t)p->reduce_size;
		for (i = 11; i <= 30; ++i)
		{
			if (reduce_size <= ((uint32_t)2 << i)) { p->dictionary_size = ((uint32_t)2 << i); break; }
			if (reduce_size <= ((uint32_t)3 << i)) { p->dictionary_size = ((uint32_t)3 << i); break; }
		}
	}

	if (p->lc < 0) p->lc = 3;
	if (p->lp < 0) p->lp = 0;
	if (p->pb < 0) p->pb = 2;

	if (p->algorithm < 0) p->algorithm = (level < 5 ? 0 : 1);
	if (p->fb < 0) p->fb = (level < 7 ? 32 : 64);
	if (p->mode_bt < 0) p->mode_bt = (p->algorithm == 0 ? 0 : 1);
	if (p->hash_byte_count < 0) p->hash_byte_count = 4;
	if (p->mc == 0) p->mc = (16 + (p->fb >> 1)) >> (p->mode_bt ? 0 : 1);

	if (p->thread_count < 0)
		p->thread_count =

		((p->mode_bt && p->algorithm) ? 2 : 1);



}

uint32_t lzma_encoder_properties_dictionary_size(const lzma_encoder_properties_t *props2)
{
	lzma_encoder_properties_t properties = *props2;
	lzma_encoder_properties_normalize(&properties);
	return properties.dictionary_size;
}




























static void LzmaEnc_FastPosInit(uint8_t *g_FastPos)
{
	uint32_t slot;
	g_FastPos[0] = 0;
	g_FastPos[1] = 1;
	g_FastPos += 2;

	for (slot = 2; slot < (9 + sizeof(size_t) / 2) * 2; slot++)
	{
		size_t k = ((size_t)1 << ((slot >> 1) - 1));
		size_t j;
		for (j = 0; j < k; j++)
			g_FastPos[j] = (uint8_t)slot;
		g_FastPos += k;
	}
}
































typedef uint32_t CState;

typedef struct
{
	uint32_t price;

	CState state;
	int32_t prev1IsChar;
	int32_t prev2;

	uint32_t posPrev2;
	uint32_t backPrev2;

	uint32_t posPrev;
	uint32_t backPrev;
	uint32_t backs[4];
} COptimal;
















































typedef struct
{
	uint16_t choice;
	uint16_t choice2;
	uint16_t low[(1 << 4) << 3];
	uint16_t mid[(1 << 4) << 3];
	uint16_t high[(1 << 8)];
} CLenEnc;


typedef struct
{
	CLenEnc p;
	uint32_t tableSize;
	uint32_t prices[(1 << 4)][((1 << 3) + (1 << 3) + (1 << 8))];
	uint32_t counters[(1 << 4)];
} CLenPriceEnc;


typedef struct
{
	uint32_t range;
	uint8_t cache;
	uint64_t low;
	uint64_t cacheSize;
	uint8_t *buffer;
	uint8_t *bufLim;
	uint8_t *bufBase;
	lzma_seq_out_stream_t *out_stream;
	uint64_t processed;
	lzma_rc_t res;
} CRangeEnc;


typedef struct
{
	uint16_t *litProbs;

	uint32_t state;
	uint32_t reps[4];

	uint16_t isMatch[12][(1 << 4)];
	uint16_t isRep[12];
	uint16_t isRepG0[12];
	uint16_t isRepG1[12];
	uint16_t isRepG2[12];
	uint16_t isRep0Long[12][(1 << 4)];

	uint16_t posSlotEncoder[4][1 << 6];
	uint16_t posEncoders[(1 << (14 >> 1)) - 14];
	uint16_t posAlignEncoder[1 << 4];

	CLenPriceEnc lenEnc;
	CLenPriceEnc repLenEnc;
} CSaveState;


typedef struct
{
	void *matchFinderObj;
	IMatchFinder matchFinder;

	uint32_t optimumEndIndex;
	uint32_t optimumCurrentIndex;

	uint32_t longestMatchLength;
	uint32_t numPairs;
	uint32_t numAvail;

	uint32_t numFastBytes;
	uint32_t additionalOffset;
	uint32_t reps[4];
	uint32_t state;

	uint32_t lc, lp, pb;
	uint32_t lp_mask, pb_mask;
	uint32_t lclp;

	uint16_t *litProbs;

	uint8_t fastMode;
	uint8_t write_end_mark;
	uint8_t finished;
	uint8_t multiThread;
	uint8_t needInit;

	uint64_t nowPos64;

	uint32_t matchPriceCount;
	uint32_t alignPriceCount;

	uint32_t distTableSize;

	uint32_t dictionary_size;
	lzma_rc_t result;

	CRangeEnc rc;
	lzma_match_finder_t matchFinderBase;
	COptimal opt[(1 << 12)];


	uint8_t g_FastPos[1 << (9 + sizeof(size_t) / 2)];


	uint32_t ProbPrices[(1 << 11) >> 4];
	uint32_t matches[(2 + ((1 << 3) + (1 << 3) + (1 << 8)) - 1) * 2 + 2 + 1];

	uint32_t posSlotPrices[4][(32 * 2)];
	uint32_t distancesPrices[4][(1 << (14 >> 1))];
	uint32_t alignPrices[(1 << 4)];

	uint16_t isMatch[12][(1 << 4)];
	uint16_t isRep[12];
	uint16_t isRepG0[12];
	uint16_t isRepG1[12];
	uint16_t isRepG2[12];
	uint16_t isRep0Long[12][(1 << 4)];

	uint16_t posSlotEncoder[4][1 << 6];
	uint16_t posEncoders[(1 << (14 >> 1)) - 14];
	uint16_t posAlignEncoder[1 << 4];

	CLenPriceEnc lenEnc;
	CLenPriceEnc repLenEnc;

	CSaveState saveState;


	uint8_t pad2[128];

} CLzmaEnc;


void LzmaEnc_SaveState(lzma_encoder_handle_t pp)
{
	CLzmaEnc *p = (CLzmaEnc *)pp;
	CSaveState *dest = &p->saveState;
	int32_t i;

	dest->lenEnc = p->lenEnc;
	dest->repLenEnc = p->repLenEnc;
	dest->state = p->state;

	for (i = 0; i < 12; ++i)
	{
		lzma_memcpy(dest->isMatch[i], p->isMatch[i], sizeof(p->isMatch[i]));
		lzma_memcpy(dest->isRep0Long[i], p->isRep0Long[i], sizeof(p->isRep0Long[i]));
	}

	for (i = 0; i < 4; ++i)
		lzma_memcpy(dest->posSlotEncoder[i], p->posSlotEncoder[i], sizeof(p->posSlotEncoder[i]));

	lzma_memcpy(dest->isRep, p->isRep, sizeof(p->isRep));
	lzma_memcpy(dest->isRepG0, p->isRepG0, sizeof(p->isRepG0));
	lzma_memcpy(dest->isRepG1, p->isRepG1, sizeof(p->isRepG1));
	lzma_memcpy(dest->isRepG2, p->isRepG2, sizeof(p->isRepG2));
	lzma_memcpy(dest->posEncoders, p->posEncoders, sizeof(p->posEncoders));
	lzma_memcpy(dest->posAlignEncoder, p->posAlignEncoder, sizeof(p->posAlignEncoder));
	lzma_memcpy(dest->reps, p->reps, sizeof(p->reps));
	lzma_memcpy(dest->litProbs, p->litProbs, ((uint32_t)0x300 << p->lclp) * sizeof(uint16_t));
}

void LzmaEnc_RestoreState(lzma_encoder_handle_t pp)
{
	CLzmaEnc *dest = (CLzmaEnc *)pp;
	const CSaveState *p = &dest->saveState;
	int32_t i;
	dest->lenEnc = p->lenEnc;
	dest->repLenEnc = p->repLenEnc;
	dest->state = p->state;

	for (i = 0; i < 12; ++i)
	{
		lzma_memcpy(dest->isMatch[i], p->isMatch[i], sizeof(p->isMatch[i]));
		lzma_memcpy(dest->isRep0Long[i], p->isRep0Long[i], sizeof(p->isRep0Long[i]));
	}

	for (i = 0; i < 4; ++i)
		lzma_memcpy(dest->posSlotEncoder[i], p->posSlotEncoder[i], sizeof(p->posSlotEncoder[i]));

	lzma_memcpy(dest->isRep, p->isRep, sizeof(p->isRep));
	lzma_memcpy(dest->isRepG0, p->isRepG0, sizeof(p->isRepG0));
	lzma_memcpy(dest->isRepG1, p->isRepG1, sizeof(p->isRepG1));
	lzma_memcpy(dest->isRepG2, p->isRepG2, sizeof(p->isRepG2));
	lzma_memcpy(dest->posEncoders, p->posEncoders, sizeof(p->posEncoders));
	lzma_memcpy(dest->posAlignEncoder, p->posAlignEncoder, sizeof(p->posAlignEncoder));
	lzma_memcpy(dest->reps, p->reps, sizeof(p->reps));
	lzma_memcpy(dest->litProbs, p->litProbs, ((uint32_t)0x300 << dest->lclp) * sizeof(uint16_t));
}

lzma_rc_t lzma_encoder_set_properties(lzma_encoder_handle_t pp, const lzma_encoder_properties_t *props2)
{
	CLzmaEnc *p = (CLzmaEnc *)pp;
	lzma_encoder_properties_t properties = *props2;
	lzma_encoder_properties_normalize(&properties);

	if (properties.lc > 8
		|| properties.lp > 4
		|| properties.pb > 4
		|| properties.dictionary_size > ((uint64_t)1 << (((9 + sizeof(size_t) / 2) - 1) * 2 + 7))
		|| properties.dictionary_size > ((uint32_t)3 << 29))
		return LZMA_RC_PARAMETER;

	p->dictionary_size = properties.dictionary_size;

	{
		uint32_t fb = properties.fb;

		if (fb < 5)
			fb = 5;

		if (fb > (2 + ((1 << 3) + (1 << 3) + (1 << 8)) - 1))
			fb = (2 + ((1 << 3) + (1 << 3) + (1 << 8)) - 1);

		p->numFastBytes = fb;
	}

	p->lc = properties.lc;
	p->lp = properties.lp;
	p->pb = properties.pb;
	p->fastMode = (properties.algorithm == 0);
	p->matchFinderBase.mode_bt = (uint8_t)(properties.mode_bt ? 1 : 0);

	{
		uint32_t hash_byte_count = 4;

		if (properties.mode_bt)
		{
			if (properties.hash_byte_count < 2)
				hash_byte_count = 2;
			else if (properties.hash_byte_count < 4)
				hash_byte_count = properties.hash_byte_count;
		}

		p->matchFinderBase.hash_byte_count = hash_byte_count;
	}

	p->matchFinderBase.cutValue = properties.mc;

	p->write_end_mark = properties.write_end_mark;

	return LZMA_RC_OK;
}


void lzma_encoder_set_data_size(lzma_encoder_handle_t pp, uint64_t expected_data_size)
{
	CLzmaEnc *p = (CLzmaEnc *)pp;
	p->matchFinderBase.expectedDataSize = expected_data_size;
}


static const int32_t kLiteralNextStates[12] = { 0, 0, 0, 0, 1, 2, 3, 4,  5,  6,   4, 5 };
static const int32_t kMatchNextStates[12] = { 7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10 };
static const int32_t kRepNextStates[12] = { 8, 8, 8, 8, 8, 8, 8, 11, 11, 11, 11, 11 };
static const int32_t kShortRepNextStates[12] = { 9, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11 };







static void RangeEnc_Construct(CRangeEnc *p)
{
	p->out_stream = ((void *)0);
	p->bufBase = ((void *)0);
}






static int32_t RangeEnc_Alloc(CRangeEnc *p, lzma_allocator_ptr_t allocator)
{
	if (!p->bufBase)
	{
		p->bufBase = (uint8_t *)(allocator)->allocate(allocator, (1 << 16));

		if (!p->bufBase)
			return 0;

		p->bufLim = p->bufBase + (1 << 16);
	}

	return 1;
}


static void RangeEnc_Free(CRangeEnc *p, lzma_allocator_ptr_t allocator)
{
	(allocator)->release(allocator, p->bufBase);
	p->bufBase = 0;
}


static void RangeEnc_Init(CRangeEnc *p)
{
	p->low = 0;
	p->range = 0xFFFFFFFF;
	p->cacheSize = 1;
	p->cache = 0;
	p->buffer = p->bufBase;
	p->processed = 0;
	p->res = LZMA_RC_OK;
}


static void RangeEnc_FlushStream(CRangeEnc *p)
{
	size_t num;
	if (p->res != LZMA_RC_OK)
		return;
	num = p->buffer - p->bufBase;
	if (num != (p->out_stream)->write(p->out_stream, p->bufBase, num))
		p->res = LZMA_RC_WRITE;
	p->processed += num;
	p->buffer = p->bufBase;
}


static void  RangeEnc_ShiftLow(CRangeEnc *p)
{
	if ((uint32_t)p->low < (uint32_t)0xFF000000 || (uint32_t)(p->low >> 32) != 0)
	{
		uint8_t temp = p->cache;
		do
		{
			uint8_t *buffer = p->buffer;
			*buffer++ = (uint8_t)(temp + (uint8_t)(p->low >> 32));
			p->buffer = buffer;

			if (buffer == p->bufLim)
				RangeEnc_FlushStream(p);

			temp = 0xFF;
		} while (--p->cacheSize != 0);

		p->cache = (uint8_t)((uint32_t)p->low >> 24);
	}
	p->cacheSize++;
	p->low = (uint32_t)p->low << 8;
}


static void RangeEnc_FlushData(CRangeEnc *p)
{
	int32_t i;
	for (i = 0; i < 5; ++i)
		RangeEnc_ShiftLow(p);
}


static void RangeEnc_EncodeDirectBits(CRangeEnc *p, uint32_t value, uint32_t numBits)
{
	do
	{
		p->range >>= 1;
		p->low += p->range & (0 - ((value >> --numBits) & 1));
		if (p->range < ((uint32_t)1 << 24))
		{
			p->range <<= 8;
			RangeEnc_ShiftLow(p);
		}
	} while (numBits != 0);
}


static void RangeEnc_EncodeBit(CRangeEnc *p, uint16_t *prob, uint32_t symbol)
{
	uint32_t ttt = *prob;
	uint32_t newBound = (p->range >> 11) * ttt;

	if (symbol == 0)
	{
		p->range = newBound;
		ttt += ((1 << 11) - ttt) >> 5;
	}
	else
	{
		p->low += newBound;
		p->range -= newBound;
		ttt -= ttt >> 5;
	}

	*prob = (uint16_t)ttt;

	if (p->range < ((uint32_t)1 << 24))
	{
		p->range <<= 8;
		RangeEnc_ShiftLow(p);
	}
}


static void LitEnc_Encode(CRangeEnc *p, uint16_t *probabilities, uint32_t symbol)
{
	symbol |= 0x100;

	do
	{
		RangeEnc_EncodeBit(p, probabilities + (symbol >> 8), (symbol >> 7) & 1);
		symbol <<= 1;
	} while (symbol < 0x10000);
}


static void LitEnc_EncodeMatched(CRangeEnc *p, uint16_t *probabilities, uint32_t symbol, uint32_t match_byte)
{
	uint32_t offs = 0x100;
	symbol |= 0x100;
	do
	{
		match_byte <<= 1;
		RangeEnc_EncodeBit(p, probabilities + (offs + (match_byte & offs) + (symbol >> 8)), (symbol >> 7) & 1);
		symbol <<= 1;
		offs &= ~(match_byte ^ symbol);
	} while (symbol < 0x10000);
}


static void LzmaEnc_InitPriceTables(uint32_t *ProbPrices)
{
	uint32_t i;

	for (i = (1 << 4) / 2; i < (1 << 11); i += (1 << 4))
	{
		const int32_t kCyclesBits = 4;
		uint32_t w = i;
		uint32_t bitCount = 0;
		int32_t j;

		for (j = 0; j < kCyclesBits; j++)
		{
			w = w * w;
			bitCount <<= 1;
			while (w >= ((uint32_t)1 << 16))
			{
				w >>= 1;
				bitCount++;
			}
		}
		ProbPrices[i >> 4] = ((11 << kCyclesBits) - 15 - bitCount);
	}
}










static uint32_t LitEnc_GetPrice(const uint16_t *probabilities, uint32_t symbol, const uint32_t *ProbPrices)
{
	uint32_t price = 0;
	symbol |= 0x100;

	do
	{
		price += ProbPrices[((probabilities[symbol >> 8]) ^ ((-((int32_t)((symbol >> 7) & 1))) & ((1 << 11) - 1))) >> 4];
		symbol <<= 1;
	} while (symbol < 0x10000);

	return price;
}


static uint32_t LitEnc_GetPriceMatched(const uint16_t *probabilities, uint32_t symbol, uint32_t match_byte, const uint32_t *ProbPrices)
{
	uint32_t price = 0;
	uint32_t offs = 0x100;
	symbol |= 0x100;

	do
	{
		match_byte <<= 1;
		price += ProbPrices[((probabilities[offs + (match_byte & offs) + (symbol >> 8)]) ^ ((-((int32_t)((symbol >> 7) & 1))) & ((1 << 11) - 1))) >> 4];
		symbol <<= 1;
		offs &= ~(match_byte ^ symbol);
	} while (symbol < 0x10000);

	return price;
}


static void RcTree_Encode(CRangeEnc *rc, uint16_t *probabilities, int32_t numBitLevels, uint32_t symbol)
{
	uint32_t m = 1;
	int32_t i;

	for (i = numBitLevels; i != 0;)
	{
		uint32_t bit;
		i--;
		bit = (symbol >> i) & 1;
		RangeEnc_EncodeBit(rc, probabilities + m, bit);
		m = (m << 1) | bit;
	}
}


static void RcTree_ReverseEncode(CRangeEnc *rc, uint16_t *probabilities, int32_t numBitLevels, uint32_t symbol)
{
	uint32_t m = 1;
	int32_t i;

	for (i = 0; i < numBitLevels; ++i)
	{
		uint32_t bit = symbol & 1;
		RangeEnc_EncodeBit(rc, probabilities + m, bit);
		m = (m << 1) | bit;
		symbol >>= 1;
	}
}


static uint32_t RcTree_GetPrice(const uint16_t *probabilities, int32_t numBitLevels, uint32_t symbol, const uint32_t *ProbPrices)
{
	uint32_t price = 0;
	symbol |= (1 << numBitLevels);

	while (symbol != 1)
	{
		price += ProbPrices[((probabilities[symbol >> 1]) ^ ((-((int32_t)(symbol & 1))) & ((1 << 11) - 1))) >> 4];
		symbol >>= 1;
	}

	return price;
}

static uint32_t RcTree_ReverseGetPrice(const uint16_t *probabilities, int32_t numBitLevels, uint32_t symbol, const uint32_t *ProbPrices)
{
	uint32_t price = 0;
	uint32_t m = 1;
	int32_t i;

	for (i = numBitLevels; i != 0; i--)
	{
		uint32_t bit = symbol & 1;
		symbol >>= 1;
		price += ProbPrices[((probabilities[m]) ^ ((-((int32_t)(bit))) & ((1 << 11) - 1))) >> 4];
		m = (m << 1) | bit;
	}

	return price;
}


static void LenEnc_Init(CLenEnc *p)
{
	uint32_t i;

	p->choice = p->choice2 = ((1 << 11) >> 1);

	for (i = 0; i < ((1 << 4) << 3); ++i)
		p->low[i] = ((1 << 11) >> 1);

	for (i = 0; i < ((1 << 4) << 3); ++i)
		p->mid[i] = ((1 << 11) >> 1);

	for (i = 0; i < (1 << 8); ++i)
		p->high[i] = ((1 << 11) >> 1);
}

static void LenEnc_Encode(CLenEnc *p, CRangeEnc *rc, uint32_t symbol, uint32_t pos_state)
{
	if (symbol < (1 << 3))
	{
		RangeEnc_EncodeBit(rc, &p->choice, 0);
		RcTree_Encode(rc, p->low + (pos_state << 3), 3, symbol);
	}
	else
	{
		RangeEnc_EncodeBit(rc, &p->choice, 1);
		if (symbol < (1 << 3) + (1 << 3))
		{
			RangeEnc_EncodeBit(rc, &p->choice2, 0);
			RcTree_Encode(rc, p->mid + (pos_state << 3), 3, symbol - (1 << 3));
		}
		else
		{
			RangeEnc_EncodeBit(rc, &p->choice2, 1);
			RcTree_Encode(rc, p->high, 8, symbol - (1 << 3) - (1 << 3));
		}
	}
}

static void LenEnc_SetPrices(CLenEnc *p, uint32_t pos_state, uint32_t numSymbols, uint32_t *prices, const uint32_t *ProbPrices)
{
	uint32_t a0 = ProbPrices[(p->choice) >> 4];
	uint32_t a1 = ProbPrices[((p->choice) ^ ((1 << 11) - 1)) >> 4];
	uint32_t b0 = a1 + ProbPrices[(p->choice2) >> 4];
	uint32_t b1 = a1 + ProbPrices[((p->choice2) ^ ((1 << 11) - 1)) >> 4];
	uint32_t i = 0;
	for (i = 0; i < (1 << 3); ++i)
	{
		if (i >= numSymbols)
			return;
		prices[i] = a0 + RcTree_GetPrice(p->low + (pos_state << 3), 3, i, ProbPrices);
	}
	for (; i < (1 << 3) + (1 << 3); ++i)
	{
		if (i >= numSymbols)
			return;
		prices[i] = b0 + RcTree_GetPrice(p->mid + (pos_state << 3), 3, i - (1 << 3), ProbPrices);
	}
	for (; i < numSymbols; ++i)
		prices[i] = b1 + RcTree_GetPrice(p->high, 8, i - (1 << 3) - (1 << 3), ProbPrices);
}

static void  LenPriceEnc_UpdateTable(CLenPriceEnc *p, uint32_t pos_state, const uint32_t *ProbPrices)
{
	LenEnc_SetPrices(&p->p, pos_state, p->tableSize, p->prices[pos_state], ProbPrices);
	p->counters[pos_state] = p->tableSize;
}

static void LenPriceEnc_UpdateTables(CLenPriceEnc *p, uint32_t numPosStates, const uint32_t *ProbPrices)
{
	uint32_t pos_state;
	for (pos_state = 0; pos_state < numPosStates; pos_state++)
		LenPriceEnc_UpdateTable(p, pos_state, ProbPrices);
}

static void LenEnc_Encode2(CLenPriceEnc *p, CRangeEnc *rc, uint32_t symbol, uint32_t pos_state, uint8_t updatePrice, const uint32_t *ProbPrices)
{
	LenEnc_Encode(&p->p, rc, symbol, pos_state);
	if (updatePrice)
		if (--p->counters[pos_state] == 0)
			LenPriceEnc_UpdateTable(p, pos_state, ProbPrices);
}




static void MovePos(CLzmaEnc *p, uint32_t num)
{





	if (num != 0)
	{
		p->additionalOffset += num;
		p->matchFinder.skip(p->matchFinderObj, num);
	}
}

static uint32_t ReadMatchDistances(CLzmaEnc *p, uint32_t *numDistancePairsRes)
{
	uint32_t lenRes = 0, numPairs;
	p->numAvail = p->matchFinder.GetNumAvailableBytes(p->matchFinderObj);
	numPairs = p->matchFinder.GetMatches(p->matchFinderObj, p->matches);











	if (numPairs > 0)
	{
		lenRes = p->matches[(size_t)numPairs - 2];
		if (lenRes == p->numFastBytes)
		{
			uint32_t numAvail = p->numAvail;
			if (numAvail > (2 + ((1 << 3) + (1 << 3) + (1 << 8)) - 1))
				numAvail = (2 + ((1 << 3) + (1 << 3) + (1 << 8)) - 1);
			{
				const uint8_t *pbyCur = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
				const uint8_t *pby = pbyCur + lenRes;
				ptrdiff_t dif = (ptrdiff_t)-1 - p->matches[(size_t)numPairs - 1];
				const uint8_t *pbyLim = pbyCur + numAvail;
				for (; pby != pbyLim && *pby == pby[dif]; pby++);
				lenRes = (uint32_t)(pby - pbyCur);
			}
		}
	}
	p->additionalOffset++;
	*numDistancePairsRes = numPairs;
	return lenRes;
}






static uint32_t GetRepLen1Price(CLzmaEnc *p, uint32_t state, uint32_t pos_state)
{
	return
		p->ProbPrices[(p->isRepG0[state]) >> 4] +
		p->ProbPrices[(p->isRep0Long[state][pos_state]) >> 4];
}

static uint32_t GetPureRepPrice(CLzmaEnc *p, uint32_t repIndex, uint32_t state, uint32_t pos_state)
{
	uint32_t price;
	if (repIndex == 0)
	{
		price = p->ProbPrices[(p->isRepG0[state]) >> 4];
		price += p->ProbPrices[((p->isRep0Long[state][pos_state]) ^ ((1 << 11) - 1)) >> 4];
	}
	else
	{
		price = p->ProbPrices[((p->isRepG0[state]) ^ ((1 << 11) - 1)) >> 4];
		if (repIndex == 1)
			price += p->ProbPrices[(p->isRepG1[state]) >> 4];
		else
		{
			price += p->ProbPrices[((p->isRepG1[state]) ^ ((1 << 11) - 1)) >> 4];
			price += p->ProbPrices[((p->isRepG2[state]) ^ (((-(int32_t)(repIndex - 2))) & ((1 << 11) - 1))) >> 4];
		}
	}
	return price;
}

static uint32_t GetRepPrice(CLzmaEnc *p, uint32_t repIndex, uint32_t len, uint32_t state, uint32_t pos_state)
{
	return p->repLenEnc.prices[pos_state][(size_t)len - 2] +
		GetPureRepPrice(p, repIndex, state, pos_state);
}

static uint32_t Backward(CLzmaEnc *p, uint32_t *backRes, uint32_t cur)
{
	uint32_t posMem = p->opt[cur].posPrev;
	uint32_t backMem = p->opt[cur].backPrev;
	p->optimumEndIndex = cur;
	do
	{
		if (p->opt[cur].prev1IsChar)
		{
			(&p->opt[posMem])->backPrev = (uint32_t)(-1); (&p->opt[posMem])->prev1IsChar = 0;
			p->opt[posMem].posPrev = posMem - 1;
			if (p->opt[cur].prev2)
			{
				p->opt[(size_t)posMem - 1].prev1IsChar = 0;
				p->opt[(size_t)posMem - 1].posPrev = p->opt[cur].posPrev2;
				p->opt[(size_t)posMem - 1].backPrev = p->opt[cur].backPrev2;
			}
		}
		{
			uint32_t posPrev = posMem;
			uint32_t backCur = backMem;

			backMem = p->opt[posPrev].backPrev;
			posMem = p->opt[posPrev].posPrev;

			p->opt[posPrev].backPrev = backCur;
			p->opt[posPrev].posPrev = cur;
			cur = posPrev;
		}
	} while (cur != 0);
	*backRes = p->opt[0].backPrev;
	p->optimumCurrentIndex = p->opt[0].posPrev;
	return p->optimumCurrentIndex;
}



static uint32_t GetOptimum(CLzmaEnc *p, uint32_t position, uint32_t *backRes)
{
	uint32_t lenEnd, cur;
	uint32_t reps[4], repLens[4];
	uint32_t *matches;

	{

		uint32_t numAvail, mainLen, numPairs, repMaxIndex, i, pos_state, len;
		uint32_t matchPrice, repMatchPrice, normalMatchPrice;
		const uint8_t *data;
		uint8_t curByte, match_byte;

		if (p->optimumEndIndex != p->optimumCurrentIndex)
		{
			const COptimal *opt = &p->opt[p->optimumCurrentIndex];
			uint32_t lenRes = opt->posPrev - p->optimumCurrentIndex;
			*backRes = opt->backPrev;
			p->optimumCurrentIndex = opt->posPrev;
			return lenRes;
		}
		p->optimumCurrentIndex = p->optimumEndIndex = 0;

		if (p->additionalOffset == 0)
			mainLen = ReadMatchDistances(p, &numPairs);
		else
		{
			mainLen = p->longestMatchLength;
			numPairs = p->numPairs;
		}

		numAvail = p->numAvail;
		if (numAvail < 2)
		{
			*backRes = (uint32_t)(-1);
			return 1;
		}
		if (numAvail > (2 + ((1 << 3) + (1 << 3) + (1 << 8)) - 1))
			numAvail = (2 + ((1 << 3) + (1 << 3) + (1 << 8)) - 1);

		data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
		repMaxIndex = 0;
		for (i = 0; i < 4; ++i)
		{
			uint32_t lenTest;
			const uint8_t *data2;
			reps[i] = p->reps[i];
			data2 = data - reps[i] - 1;
			if (data[0] != data2[0] || data[1] != data2[1])
			{
				repLens[i] = 0;
				continue;
			}
			for (lenTest = 2; lenTest < numAvail && data[lenTest] == data2[lenTest]; lenTest++);
			repLens[i] = lenTest;
			if (lenTest > repLens[repMaxIndex])
				repMaxIndex = i;
		}
		if (repLens[repMaxIndex] >= p->numFastBytes)
		{
			uint32_t lenRes;
			*backRes = repMaxIndex;
			lenRes = repLens[repMaxIndex];
			MovePos(p, lenRes - 1);
			return lenRes;
		}

		matches = p->matches;
		if (mainLen >= p->numFastBytes)
		{
			*backRes = matches[(size_t)numPairs - 1] + 4;
			MovePos(p, mainLen - 1);
			return mainLen;
		}
		curByte = *data;
		match_byte = *(data - (reps[0] + 1));

		if (mainLen < 2 && curByte != match_byte && repLens[repMaxIndex] < 2)
		{
			*backRes = (uint32_t)-1;
			return 1;
		}

		p->opt[0].state = (CState)p->state;

		pos_state = (position & p->pb_mask);

		{
			const uint16_t *probabilities = (p->litProbs + ((((position)& p->lp_mask) << p->lc) + ((*(data - 1)) >> (8 - p->lc))) * (uint32_t)0x300);
			p->opt[1].price = p->ProbPrices[(p->isMatch[p->state][pos_state]) >> 4] +
				(!((p->state) < 7) ?
					LitEnc_GetPriceMatched(probabilities, curByte, match_byte, p->ProbPrices) :
					LitEnc_GetPrice(probabilities, curByte, p->ProbPrices));
		}

		(&p->opt[1])->backPrev = (uint32_t)(-1); (&p->opt[1])->prev1IsChar = 0;

		matchPrice = p->ProbPrices[((p->isMatch[p->state][pos_state]) ^ ((1 << 11) - 1)) >> 4];
		repMatchPrice = matchPrice + p->ProbPrices[((p->isRep[p->state]) ^ ((1 << 11) - 1)) >> 4];

		if (match_byte == curByte)
		{
			uint32_t shortRepPrice = repMatchPrice + GetRepLen1Price(p, p->state, pos_state);
			if (shortRepPrice < p->opt[1].price)
			{
				p->opt[1].price = shortRepPrice;
				(&p->opt[1])->backPrev = 0; (&p->opt[1])->prev1IsChar = 0;
			}
		}
		lenEnd = ((mainLen >= repLens[repMaxIndex]) ? mainLen : repLens[repMaxIndex]);

		if (lenEnd < 2)
		{
			*backRes = p->opt[1].backPrev;
			return 1;
		}

		p->opt[1].posPrev = 0;
		for (i = 0; i < 4; ++i)
			p->opt[0].backs[i] = reps[i];

		len = lenEnd;
		do
			p->opt[len--].price = (1 << 30);
		while (len >= 2);

		for (i = 0; i < 4; ++i)
		{
			uint32_t repLen = repLens[i];
			uint32_t price;
			if (repLen < 2)
				continue;
			price = repMatchPrice + GetPureRepPrice(p, i, p->state, pos_state);
			do
			{
				uint32_t curAndLenPrice = price + p->repLenEnc.prices[pos_state][(size_t)repLen - 2];
				COptimal *opt = &p->opt[repLen];
				if (curAndLenPrice < opt->price)
				{
					opt->price = curAndLenPrice;
					opt->posPrev = 0;
					opt->backPrev = i;
					opt->prev1IsChar = 0;
				}
			} while (--repLen >= 2);
		}

		normalMatchPrice = matchPrice + p->ProbPrices[(p->isRep[p->state]) >> 4];

		len = ((repLens[0] >= 2) ? repLens[0] + 1 : 2);
		if (len <= mainLen)
		{
			uint32_t offs = 0;
			while (len > matches[offs])
				offs += 2;
			for (; ; len++)
			{
				COptimal *opt;
				uint32_t distance = matches[(size_t)offs + 1];

				uint32_t curAndLenPrice = normalMatchPrice + p->lenEnc.prices[pos_state][(size_t)len - 2];
				uint32_t lenToPosState = (((len) < 4 + 1) ? (len)-2 : 4 - 1);
				if (distance < (1 << (14 >> 1)))
					curAndLenPrice += p->distancesPrices[lenToPosState][distance];
				else
				{
					uint32_t slot;
					{ { uint32_t zz = (distance < (1 << ((9 + sizeof(size_t) / 2) + 6))) ? 6 : 6 + (9 + sizeof(size_t) / 2) - 1; slot = p->g_FastPos[distance >> zz] + (zz * 2); }; };
					curAndLenPrice += p->alignPrices[distance & ((1 << 4) - 1)] + p->posSlotPrices[lenToPosState][slot];
				}
				opt = &p->opt[len];
				if (curAndLenPrice < opt->price)
				{
					opt->price = curAndLenPrice;
					opt->posPrev = 0;
					opt->backPrev = distance + 4;
					opt->prev1IsChar = 0;
				}
				if (len == matches[offs])
				{
					offs += 2;
					if (offs == numPairs)
						break;
				}
			}
		}

		cur = 0;
	}

	for (;;)
	{
		uint32_t numAvail;
		uint32_t numAvailFull, newLen, numPairs, posPrev, state, pos_state, startLen;
		uint32_t curPrice, curAnd1Price, matchPrice, repMatchPrice;
		uint8_t nextIsChar;
		uint8_t curByte, match_byte;
		const uint8_t *data;
		COptimal *curOpt;
		COptimal *nextOpt;

		cur++;
		if (cur == lenEnd)
			return Backward(p, backRes, cur);

		newLen = ReadMatchDistances(p, &numPairs);
		if (newLen >= p->numFastBytes)
		{
			p->numPairs = numPairs;
			p->longestMatchLength = newLen;
			return Backward(p, backRes, cur);
		}
		position++;
		curOpt = &p->opt[cur];
		posPrev = curOpt->posPrev;
		if (curOpt->prev1IsChar)
		{
			posPrev--;
			if (curOpt->prev2)
			{
				state = p->opt[curOpt->posPrev2].state;
				if (curOpt->backPrev2 < 4)
					state = kRepNextStates[state];
				else
					state = kMatchNextStates[state];
			}
			else
				state = p->opt[posPrev].state;
			state = kLiteralNextStates[state];
		}
		else
			state = p->opt[posPrev].state;
		if (posPrev == cur - 1)
		{
			if (((curOpt)->backPrev == 0))
				state = kShortRepNextStates[state];
			else
				state = kLiteralNextStates[state];
		}
		else
		{
			uint32_t pos;
			const COptimal *prevOpt;
			if (curOpt->prev1IsChar && curOpt->prev2)
			{
				posPrev = curOpt->posPrev2;
				pos = curOpt->backPrev2;
				state = kRepNextStates[state];
			}
			else
			{
				pos = curOpt->backPrev;
				if (pos < 4)
					state = kRepNextStates[state];
				else
					state = kMatchNextStates[state];
			}
			prevOpt = &p->opt[posPrev];
			if (pos < 4)
			{
				uint32_t i;
				reps[0] = prevOpt->backs[pos];
				for (i = 1; i <= pos; ++i)
					reps[i] = prevOpt->backs[(size_t)i - 1];
				for (; i < 4; ++i)
					reps[i] = prevOpt->backs[i];
			}
			else
			{
				uint32_t i;
				reps[0] = (pos - 4);
				for (i = 1; i < 4; ++i)
					reps[i] = prevOpt->backs[(size_t)i - 1];
			}
		}
		curOpt->state = (CState)state;

		curOpt->backs[0] = reps[0];
		curOpt->backs[1] = reps[1];
		curOpt->backs[2] = reps[2];
		curOpt->backs[3] = reps[3];

		curPrice = curOpt->price;
		nextIsChar = 0;
		data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
		curByte = *data;
		match_byte = *(data - (reps[0] + 1));

		pos_state = (position & p->pb_mask);

		curAnd1Price = curPrice + p->ProbPrices[(p->isMatch[state][pos_state]) >> 4];
		{
			const uint16_t *probabilities = (p->litProbs + ((((position)& p->lp_mask) << p->lc) + ((*(data - 1)) >> (8 - p->lc))) * (uint32_t)0x300);
			curAnd1Price +=
				(!((state) < 7) ?
					LitEnc_GetPriceMatched(probabilities, curByte, match_byte, p->ProbPrices) :
					LitEnc_GetPrice(probabilities, curByte, p->ProbPrices));
		}

		nextOpt = &p->opt[(size_t)cur + 1];

		if (curAnd1Price < nextOpt->price)
		{
			nextOpt->price = curAnd1Price;
			nextOpt->posPrev = cur;
			(nextOpt)->backPrev = (uint32_t)(-1); (nextOpt)->prev1IsChar = 0;
			nextIsChar = 1;
		}

		matchPrice = curPrice + p->ProbPrices[((p->isMatch[state][pos_state]) ^ ((1 << 11) - 1)) >> 4];
		repMatchPrice = matchPrice + p->ProbPrices[((p->isRep[state]) ^ ((1 << 11) - 1)) >> 4];

		if (match_byte == curByte && !(nextOpt->posPrev < cur && nextOpt->backPrev == 0))
		{
			uint32_t shortRepPrice = repMatchPrice + GetRepLen1Price(p, state, pos_state);
			if (shortRepPrice <= nextOpt->price)
			{
				nextOpt->price = shortRepPrice;
				nextOpt->posPrev = cur;
				(nextOpt)->backPrev = 0; (nextOpt)->prev1IsChar = 0;
				nextIsChar = 1;
			}
		}
		numAvailFull = p->numAvail;
		{
			uint32_t temp = (1 << 12) - 1 - cur;
			if (temp < numAvailFull)
				numAvailFull = temp;
		}

		if (numAvailFull < 2)
			continue;
		numAvail = (numAvailFull <= p->numFastBytes ? numAvailFull : p->numFastBytes);

		if (!nextIsChar && match_byte != curByte)
		{

			uint32_t temp;
			uint32_t lenTest2;
			const uint8_t *data2 = data - reps[0] - 1;
			uint32_t limit = p->numFastBytes + 1;
			if (limit > numAvailFull)
				limit = numAvailFull;

			for (temp = 1; temp < limit && data[temp] == data2[temp]; temp++);
			lenTest2 = temp - 1;
			if (lenTest2 >= 2)
			{
				uint32_t state2 = kLiteralNextStates[state];
				uint32_t posStateNext = (position + 1) & p->pb_mask;
				uint32_t nextRepMatchPrice = curAnd1Price +
					p->ProbPrices[((p->isMatch[state2][posStateNext]) ^ ((1 << 11) - 1)) >> 4] +
					p->ProbPrices[((p->isRep[state2]) ^ ((1 << 11) - 1)) >> 4];

				{
					uint32_t curAndLenPrice;
					COptimal *opt;
					uint32_t offset = cur + 1 + lenTest2;
					while (lenEnd < offset)
						p->opt[++lenEnd].price = (1 << 30);
					curAndLenPrice = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
					opt = &p->opt[offset];
					if (curAndLenPrice < opt->price)
					{
						opt->price = curAndLenPrice;
						opt->posPrev = cur + 1;
						opt->backPrev = 0;
						opt->prev1IsChar = 1;
						opt->prev2 = 0;
					}
				}
			}
		}

		startLen = 2;
		{
			uint32_t repIndex;
			for (repIndex = 0; repIndex < 4; repIndex++)
			{
				uint32_t lenTest;
				uint32_t lenTestTemp;
				uint32_t price;
				const uint8_t *data2 = data - reps[repIndex] - 1;
				if (data[0] != data2[0] || data[1] != data2[1])
					continue;
				for (lenTest = 2; lenTest < numAvail && data[lenTest] == data2[lenTest]; lenTest++);
				while (lenEnd < cur + lenTest)
					p->opt[++lenEnd].price = (1 << 30);
				lenTestTemp = lenTest;
				price = repMatchPrice + GetPureRepPrice(p, repIndex, state, pos_state);
				do
				{
					uint32_t curAndLenPrice = price + p->repLenEnc.prices[pos_state][(size_t)lenTest - 2];
					COptimal *opt = &p->opt[cur + lenTest];
					if (curAndLenPrice < opt->price)
					{
						opt->price = curAndLenPrice;
						opt->posPrev = cur;
						opt->backPrev = repIndex;
						opt->prev1IsChar = 0;
					}
				} while (--lenTest >= 2);
				lenTest = lenTestTemp;

				if (repIndex == 0)
					startLen = lenTest + 1;


				{
					uint32_t lenTest2 = lenTest + 1;
					uint32_t limit = lenTest2 + p->numFastBytes;
					if (limit > numAvailFull)
						limit = numAvailFull;
					for (; lenTest2 < limit && data[lenTest2] == data2[lenTest2]; lenTest2++);
					lenTest2 -= lenTest + 1;
					if (lenTest2 >= 2)
					{
						uint32_t nextRepMatchPrice;
						uint32_t state2 = kRepNextStates[state];
						uint32_t posStateNext = (position + lenTest) & p->pb_mask;
						uint32_t curAndLenCharPrice =
							price + p->repLenEnc.prices[pos_state][(size_t)lenTest - 2] +
							p->ProbPrices[(p->isMatch[state2][posStateNext]) >> 4] +
							LitEnc_GetPriceMatched((p->litProbs + ((((position + lenTest) & p->lp_mask) << p->lc) + ((data[(size_t)lenTest - 1]) >> (8 - p->lc))) * (uint32_t)0x300),
								data[lenTest], data2[lenTest], p->ProbPrices);
						state2 = kLiteralNextStates[state2];
						posStateNext = (position + lenTest + 1) & p->pb_mask;
						nextRepMatchPrice = curAndLenCharPrice +
							p->ProbPrices[((p->isMatch[state2][posStateNext]) ^ ((1 << 11) - 1)) >> 4] +
							p->ProbPrices[((p->isRep[state2]) ^ ((1 << 11) - 1)) >> 4];


						{
							uint32_t curAndLenPrice;
							COptimal *opt;
							uint32_t offset = cur + lenTest + 1 + lenTest2;
							while (lenEnd < offset)
								p->opt[++lenEnd].price = (1 << 30);
							curAndLenPrice = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
							opt = &p->opt[offset];
							if (curAndLenPrice < opt->price)
							{
								opt->price = curAndLenPrice;
								opt->posPrev = cur + lenTest + 1;
								opt->backPrev = 0;
								opt->prev1IsChar = 1;
								opt->prev2 = 1;
								opt->posPrev2 = cur;
								opt->backPrev2 = repIndex;
							}
						}
					}
				}
			}
		}

		if (newLen > numAvail)
		{
			newLen = numAvail;
			for (numPairs = 0; newLen > matches[numPairs]; numPairs += 2);
			matches[numPairs] = newLen;
			numPairs += 2;
		}
		if (newLen >= startLen)
		{
			uint32_t normalMatchPrice = matchPrice + p->ProbPrices[(p->isRep[state]) >> 4];
			uint32_t offs, curBack, pos_slot;
			uint32_t lenTest;
			while (lenEnd < cur + newLen)
				p->opt[++lenEnd].price = (1 << 30);

			offs = 0;
			while (startLen > matches[offs])
				offs += 2;
			curBack = matches[(size_t)offs + 1];
			{ { uint32_t zz = (curBack < (1 << ((9 + sizeof(size_t) / 2) + 6))) ? 6 : 6 + (9 + sizeof(size_t) / 2) - 1; pos_slot = p->g_FastPos[curBack >> zz] + (zz * 2); }; };
			for (lenTest = startLen; ; lenTest++)
			{
				uint32_t curAndLenPrice = normalMatchPrice + p->lenEnc.prices[pos_state][(size_t)lenTest - 2];
				{
					uint32_t lenToPosState = (((lenTest) < 4 + 1) ? (lenTest)-2 : 4 - 1);
					COptimal *opt;
					if (curBack < (1 << (14 >> 1)))
						curAndLenPrice += p->distancesPrices[lenToPosState][curBack];
					else
						curAndLenPrice += p->posSlotPrices[lenToPosState][pos_slot] + p->alignPrices[curBack & ((1 << 4) - 1)];

					opt = &p->opt[cur + lenTest];
					if (curAndLenPrice < opt->price)
					{
						opt->price = curAndLenPrice;
						opt->posPrev = cur;
						opt->backPrev = curBack + 4;
						opt->prev1IsChar = 0;
					}
				}

				if (lenTest == matches[offs])
				{

					const uint8_t *data2 = data - curBack - 1;
					uint32_t lenTest2 = lenTest + 1;
					uint32_t limit = lenTest2 + p->numFastBytes;
					if (limit > numAvailFull)
						limit = numAvailFull;
					for (; lenTest2 < limit && data[lenTest2] == data2[lenTest2]; lenTest2++);
					lenTest2 -= lenTest + 1;
					if (lenTest2 >= 2)
					{
						uint32_t nextRepMatchPrice;
						uint32_t state2 = kMatchNextStates[state];
						uint32_t posStateNext = (position + lenTest) & p->pb_mask;
						uint32_t curAndLenCharPrice = curAndLenPrice +
							p->ProbPrices[(p->isMatch[state2][posStateNext]) >> 4] +
							LitEnc_GetPriceMatched((p->litProbs + ((((position + lenTest) & p->lp_mask) << p->lc) + ((data[(size_t)lenTest - 1]) >> (8 - p->lc))) * (uint32_t)0x300),
								data[lenTest], data2[lenTest], p->ProbPrices);
						state2 = kLiteralNextStates[state2];
						posStateNext = (posStateNext + 1) & p->pb_mask;
						nextRepMatchPrice = curAndLenCharPrice +
							p->ProbPrices[((p->isMatch[state2][posStateNext]) ^ ((1 << 11) - 1)) >> 4] +
							p->ProbPrices[((p->isRep[state2]) ^ ((1 << 11) - 1)) >> 4];


						{
							uint32_t offset = cur + lenTest + 1 + lenTest2;
							uint32_t curAndLenPrice2;
							COptimal *opt;
							while (lenEnd < offset)
								p->opt[++lenEnd].price = (1 << 30);
							curAndLenPrice2 = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
							opt = &p->opt[offset];
							if (curAndLenPrice2 < opt->price)
							{
								opt->price = curAndLenPrice2;
								opt->posPrev = cur + lenTest + 1;
								opt->backPrev = 0;
								opt->prev1IsChar = 1;
								opt->prev2 = 1;
								opt->posPrev2 = cur;
								opt->backPrev2 = curBack + 4;
							}
						}
					}
					offs += 2;
					if (offs == numPairs)
						break;
					curBack = matches[(size_t)offs + 1];
					if (curBack >= (1 << (14 >> 1)))
					{ { uint32_t zz = (curBack < (1 << ((9 + sizeof(size_t) / 2) + 6))) ? 6 : 6 + (9 + sizeof(size_t) / 2) - 1; pos_slot = p->g_FastPos[curBack >> zz] + (zz * 2); };
					};
				}
			}
		}
	}
}



static uint32_t GetOptimumFast(CLzmaEnc *p, uint32_t *backRes)
{
	uint32_t numAvail, mainLen, mainDist, numPairs, repIndex, repLen, i;
	const uint8_t *data;
	const uint32_t *matches;

	if (p->additionalOffset == 0)
		mainLen = ReadMatchDistances(p, &numPairs);
	else
	{
		mainLen = p->longestMatchLength;
		numPairs = p->numPairs;
	}

	numAvail = p->numAvail;
	*backRes = (uint32_t)-1;
	if (numAvail < 2)
		return 1;
	if (numAvail > (2 + ((1 << 3) + (1 << 3) + (1 << 8)) - 1))
		numAvail = (2 + ((1 << 3) + (1 << 3) + (1 << 8)) - 1);
	data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;

	repLen = repIndex = 0;
	for (i = 0; i < 4; ++i)
	{
		uint32_t len;
		const uint8_t *data2 = data - p->reps[i] - 1;
		if (data[0] != data2[0] || data[1] != data2[1])
			continue;
		for (len = 2; len < numAvail && data[len] == data2[len]; len++);
		if (len >= p->numFastBytes)
		{
			*backRes = i;
			MovePos(p, len - 1);
			return len;
		}
		if (len > repLen)
		{
			repIndex = i;
			repLen = len;
		}
	}

	matches = p->matches;
	if (mainLen >= p->numFastBytes)
	{
		*backRes = matches[(size_t)numPairs - 1] + 4;
		MovePos(p, mainLen - 1);
		return mainLen;
	}

	mainDist = 0;
	if (mainLen >= 2)
	{
		mainDist = matches[(size_t)numPairs - 1];
		while (numPairs > 2 && mainLen == matches[(size_t)numPairs - 4] + 1)
		{
			if (!(((mainDist) >> 7) > (matches[(size_t)numPairs - 3])))
				break;
			numPairs -= 2;
			mainLen = matches[(size_t)numPairs - 2];
			mainDist = matches[(size_t)numPairs - 1];
		}
		if (mainLen == 2 && mainDist >= 0x80)
			mainLen = 1;
	}

	if (repLen >= 2 && (
		(repLen + 1 >= mainLen) ||
		(repLen + 2 >= mainLen && mainDist >= (1 << 9)) ||
		(repLen + 3 >= mainLen && mainDist >= (1 << 15))))
	{
		*backRes = repIndex;
		MovePos(p, repLen - 1);
		return repLen;
	}

	if (mainLen < 2 || numAvail <= 2)
		return 1;

	p->longestMatchLength = ReadMatchDistances(p, &p->numPairs);
	if (p->longestMatchLength >= 2)
	{
		uint32_t newDistance = matches[(size_t)p->numPairs - 1];
		if ((p->longestMatchLength >= mainLen && newDistance < mainDist) ||
			(p->longestMatchLength == mainLen + 1 && !(((newDistance) >> 7) > (mainDist))) ||
			(p->longestMatchLength > mainLen + 1) ||
			(p->longestMatchLength + 1 >= mainLen && mainLen >= 3 && (((mainDist) >> 7) > (newDistance))))
			return 1;
	}

	data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
	for (i = 0; i < 4; ++i)
	{
		uint32_t len, limit;
		const uint8_t *data2 = data - p->reps[i] - 1;
		if (data[0] != data2[0] || data[1] != data2[1])
			continue;
		limit = mainLen - 1;
		for (len = 2; len < limit && data[len] == data2[len]; len++);
		if (len >= limit)
			return 1;
	}
	*backRes = mainDist + 4;
	MovePos(p, mainLen - 2);
	return mainLen;
}

static void WriteEndMarker(CLzmaEnc *p, uint32_t pos_state)
{
	uint32_t len;
	RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][pos_state], 1);
	RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 0);
	p->state = kMatchNextStates[p->state];
	len = 2;
	LenEnc_Encode2(&p->lenEnc, &p->rc, len - 2, pos_state, !p->fastMode, p->ProbPrices);
	RcTree_Encode(&p->rc, p->posSlotEncoder[(((len) < 4 + 1) ? (len)-2 : 4 - 1)], 6, (1 << 6) - 1);
	RangeEnc_EncodeDirectBits(&p->rc, (((uint32_t)1 << 30) - 1) >> 4, 30 - 4);
	RcTree_ReverseEncode(&p->rc, p->posAlignEncoder, 4, ((1 << 4) - 1));
}

static lzma_rc_t CheckErrors(CLzmaEnc *p)
{
	if (p->result != LZMA_RC_OK)
		return p->result;
	if (p->rc.res != LZMA_RC_OK)
		p->result = LZMA_RC_WRITE;
	if (p->matchFinderBase.result != LZMA_RC_OK)
		p->result = LZMA_RC_READ;
	if (p->result != LZMA_RC_OK)
		p->finished = 1;
	return p->result;
}

static lzma_rc_t Flush(CLzmaEnc *p, uint32_t nowPos)
{

	p->finished = 1;
	if (p->write_end_mark)
		WriteEndMarker(p, nowPos & p->pb_mask);
	RangeEnc_FlushData(&p->rc);
	RangeEnc_FlushStream(&p->rc);
	return CheckErrors(p);
}

static void FillAlignPrices(CLzmaEnc *p)
{
	uint32_t i;
	for (i = 0; i < (1 << 4); ++i)
		p->alignPrices[i] = RcTree_ReverseGetPrice(p->posAlignEncoder, 4, i, p->ProbPrices);
	p->alignPriceCount = 0;
}

static void FillDistancesPrices(CLzmaEnc *p)
{
	uint32_t tempPrices[(1 << (14 >> 1))];
	uint32_t i, lenToPosState;
	for (i = 4; i < (1 << (14 >> 1)); ++i)
	{
		uint32_t pos_slot = p->g_FastPos[i];
		uint32_t footerBits = ((pos_slot >> 1) - 1);
		uint32_t base = ((2 | (pos_slot & 1)) << footerBits);
		tempPrices[i] = RcTree_ReverseGetPrice(p->posEncoders + base - pos_slot - 1, footerBits, i - base, p->ProbPrices);
	}

	for (lenToPosState = 0; lenToPosState < 4; lenToPosState++)
	{
		uint32_t pos_slot;
		const uint16_t *encoder = p->posSlotEncoder[lenToPosState];
		uint32_t *posSlotPrices = p->posSlotPrices[lenToPosState];
		for (pos_slot = 0; pos_slot < p->distTableSize; pos_slot++)
			posSlotPrices[pos_slot] = RcTree_GetPrice(encoder, 6, pos_slot, p->ProbPrices);
		for (pos_slot = 14; pos_slot < p->distTableSize; pos_slot++)
			posSlotPrices[pos_slot] += ((((pos_slot >> 1) - 1) - 4) << 4);

		{
			uint32_t *distancesPrices = p->distancesPrices[lenToPosState];
			for (i = 0; i < 4; ++i)
				distancesPrices[i] = posSlotPrices[i];
			for (; i < (1 << (14 >> 1)); ++i)
				distancesPrices[i] = posSlotPrices[p->g_FastPos[i]] + tempPrices[i];
		}
	}
	p->matchPriceCount = 0;
}

void LzmaEnc_Construct(CLzmaEnc *p)
{
	RangeEnc_Construct(&p->rc);
	MatchFinder_Construct(&p->matchFinderBase);

	{
		lzma_encoder_properties_t properties;
		lzma_encoder_properties_init(&properties);
		lzma_encoder_set_properties(p, &properties);
	}


	LzmaEnc_FastPosInit(p->g_FastPos);


	LzmaEnc_InitPriceTables(p->ProbPrices);
	p->litProbs = ((void *)0);
	p->saveState.litProbs = ((void *)0);
}

lzma_encoder_handle_t lzma_encoder_create(lzma_allocator_ptr_t allocator)
{
	void *p;
	p = (allocator)->allocate(allocator, sizeof(CLzmaEnc));
	if (p)
		LzmaEnc_Construct((CLzmaEnc *)p);
	return p;
}

void LzmaEnc_FreeLits(CLzmaEnc *p, lzma_allocator_ptr_t allocator)
{
	(allocator)->release(allocator, p->litProbs);
	(allocator)->release(allocator, p->saveState.litProbs);
	p->litProbs = ((void *)0);
	p->saveState.litProbs = ((void *)0);
}

void LzmaEnc_Destruct(CLzmaEnc *p, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	MatchFinder_Free(&p->matchFinderBase, allocator_large);
	LzmaEnc_FreeLits(p, allocator);
	RangeEnc_Free(&p->rc, allocator);
}

void lzma_encoder_destroy(lzma_encoder_handle_t p, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	LzmaEnc_Destruct((CLzmaEnc *)p, allocator, allocator_large);
	(allocator)->release(allocator, p);
}

static lzma_rc_t LzmaEnc_CodeOneBlock(CLzmaEnc *p, uint8_t useLimits, uint32_t maxPackSize, uint32_t maxUnpackSize)
{
	uint32_t nowPos32, startPos32;
	if (p->needInit)
	{
		p->matchFinder.Init(p->matchFinderObj);
		p->needInit = 0;
	}

	if (p->finished)
		return p->result;
	{ lzma_rc_t rc__ = (CheckErrors(p)); if (rc__ != 0) return rc__; };

	nowPos32 = (uint32_t)p->nowPos64;
	startPos32 = nowPos32;

	if (p->nowPos64 == 0)
	{
		uint32_t numPairs;
		uint8_t curByte;
		if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) == 0)
			return Flush(p, nowPos32);
		ReadMatchDistances(p, &numPairs);
		RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][0], 0);
		p->state = kLiteralNextStates[p->state];
		curByte = *(p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset);
		LitEnc_Encode(&p->rc, p->litProbs, curByte);
		p->additionalOffset--;
		nowPos32++;
	}

	if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) != 0)
		for (;;)
		{
			uint32_t pos, len, pos_state;

			if (p->fastMode)
				len = GetOptimumFast(p, &pos);
			else
				len = GetOptimum(p, nowPos32, &pos);





			pos_state = nowPos32 & p->pb_mask;
			if (len == 1 && pos == (uint32_t)-1)
			{
				uint8_t curByte;
				uint16_t *probabilities;
				const uint8_t *data;

				RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][pos_state], 0);
				data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset;
				curByte = *data;
				probabilities = (p->litProbs + ((((nowPos32)& p->lp_mask) << p->lc) + ((*(data - 1)) >> (8 - p->lc))) * (uint32_t)0x300);
				if (((p->state) < 7))
					LitEnc_Encode(&p->rc, probabilities, curByte);
				else
					LitEnc_EncodeMatched(&p->rc, probabilities, curByte, *(data - p->reps[0] - 1));
				p->state = kLiteralNextStates[p->state];
			}
			else
			{
				RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][pos_state], 1);
				if (pos < 4)
				{
					RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 1);
					if (pos == 0)
					{
						RangeEnc_EncodeBit(&p->rc, &p->isRepG0[p->state], 0);
						RangeEnc_EncodeBit(&p->rc, &p->isRep0Long[p->state][pos_state], ((len == 1) ? 0 : 1));
					}
					else
					{
						uint32_t distance = p->reps[pos];
						RangeEnc_EncodeBit(&p->rc, &p->isRepG0[p->state], 1);
						if (pos == 1)
							RangeEnc_EncodeBit(&p->rc, &p->isRepG1[p->state], 0);
						else
						{
							RangeEnc_EncodeBit(&p->rc, &p->isRepG1[p->state], 1);
							RangeEnc_EncodeBit(&p->rc, &p->isRepG2[p->state], pos - 2);
							if (pos == 3)
								p->reps[3] = p->reps[2];
							p->reps[2] = p->reps[1];
						}
						p->reps[1] = p->reps[0];
						p->reps[0] = distance;
					}
					if (len == 1)
						p->state = kShortRepNextStates[p->state];
					else
					{
						LenEnc_Encode2(&p->repLenEnc, &p->rc, len - 2, pos_state, !p->fastMode, p->ProbPrices);
						p->state = kRepNextStates[p->state];
					}
				}
				else
				{
					uint32_t pos_slot;
					RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 0);
					p->state = kMatchNextStates[p->state];
					LenEnc_Encode2(&p->lenEnc, &p->rc, len - 2, pos_state, !p->fastMode, p->ProbPrices);
					pos -= 4;
					{ if (pos < (1 << (14 >> 1))) pos_slot = p->g_FastPos[pos]; else { uint32_t zz = (pos < (1 << ((9 + sizeof(size_t) / 2) + 6))) ? 6 : 6 + (9 + sizeof(size_t) / 2) - 1; pos_slot = p->g_FastPos[pos >> zz] + (zz * 2); }; };
					RcTree_Encode(&p->rc, p->posSlotEncoder[(((len) < 4 + 1) ? (len)-2 : 4 - 1)], 6, pos_slot);

					if (pos_slot >= 4)
					{
						uint32_t footerBits = ((pos_slot >> 1) - 1);
						uint32_t base = ((2 | (pos_slot & 1)) << footerBits);
						uint32_t posReduced = pos - base;

						if (pos_slot < 14)
							RcTree_ReverseEncode(&p->rc, p->posEncoders + base - pos_slot - 1, footerBits, posReduced);
						else
						{
							RangeEnc_EncodeDirectBits(&p->rc, posReduced >> 4, footerBits - 4);
							RcTree_ReverseEncode(&p->rc, p->posAlignEncoder, 4, posReduced & ((1 << 4) - 1));
							p->alignPriceCount++;
						}
					}
					p->reps[3] = p->reps[2];
					p->reps[2] = p->reps[1];
					p->reps[1] = p->reps[0];
					p->reps[0] = pos;
					p->matchPriceCount++;
				}
			}
			p->additionalOffset -= len;
			nowPos32 += len;
			if (p->additionalOffset == 0)
			{
				uint32_t processed;
				if (!p->fastMode)
				{
					if (p->matchPriceCount >= (1 << 7))
						FillDistancesPrices(p);
					if (p->alignPriceCount >= (1 << 4))
						FillAlignPrices(p);
				}
				if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) == 0)
					break;
				processed = nowPos32 - startPos32;
				if (useLimits)
				{
					if (processed + (1 << 12) + 300 >= maxUnpackSize ||
						((&p->rc)->processed + ((&p->rc)->buffer - (&p->rc)->bufBase) + (&p->rc)->cacheSize) + (1 << 12) * 2 >= maxPackSize)
						break;
				}
				else if (processed >= (1 << 17))
				{
					p->nowPos64 += nowPos32 - startPos32;
					return CheckErrors(p);
				}
			}
		}
	p->nowPos64 += nowPos32 - startPos32;
	return Flush(p, nowPos32);
}



static lzma_rc_t LzmaEnc_Alloc(CLzmaEnc *p, uint32_t keepWindowSize, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	uint32_t beforeSize = (1 << 12);

	if (!RangeEnc_Alloc(&p->rc, allocator))
		return LZMA_RC_MEMORY;

	{
		uint32_t lclp = p->lc + p->lp;

		if (!p->litProbs || !p->saveState.litProbs || p->lclp != lclp)
		{
			LzmaEnc_FreeLits(p, allocator);
			p->litProbs = (uint16_t *)(allocator)->allocate(allocator, ((uint32_t)0x300 << lclp) * sizeof(uint16_t));
			p->saveState.litProbs = (uint16_t *)(allocator)->allocate(allocator, ((uint32_t)0x300 << lclp) * sizeof(uint16_t));

			if (!p->litProbs || !p->saveState.litProbs)
			{
				LzmaEnc_FreeLits(p, allocator);
				return LZMA_RC_MEMORY;
			}

			p->lclp = lclp;
		}
	}

	p->matchFinderBase.bigHash = (uint8_t)(p->dictionary_size > ((uint32_t)1 << 24) ? 1 : 0);

	if (beforeSize + p->dictionary_size < keepWindowSize)
		beforeSize = keepWindowSize - p->dictionary_size;

	{
		if (!MatchFinder_Create(&p->matchFinderBase, p->dictionary_size, beforeSize, p->numFastBytes, (2 + ((1 << 3) + (1 << 3) + (1 << 8)) - 1), allocator_large))
			return LZMA_RC_MEMORY;
		p->matchFinderObj = &p->matchFinderBase;
		MatchFinder_CreateVTable(&p->matchFinderBase, &p->matchFinder);
	}

	return LZMA_RC_OK;
}

void LzmaEnc_Init(CLzmaEnc *p)
{
	uint32_t i;
	p->state = 0;
	for (i = 0; i < 4; ++i)
		p->reps[i] = 0;

	RangeEnc_Init(&p->rc);


	for (i = 0; i < 12; ++i)
	{
		uint32_t j;
		for (j = 0; j < (1 << 4); j++)
		{
			p->isMatch[i][j] = ((1 << 11) >> 1);
			p->isRep0Long[i][j] = ((1 << 11) >> 1);
		}
		p->isRep[i] = ((1 << 11) >> 1);
		p->isRepG0[i] = ((1 << 11) >> 1);
		p->isRepG1[i] = ((1 << 11) >> 1);
		p->isRepG2[i] = ((1 << 11) >> 1);
	}

	{
		uint32_t num = (uint32_t)0x300 << (p->lp + p->lc);
		uint16_t *probabilities = p->litProbs;
		for (i = 0; i < num; ++i)
			probabilities[i] = ((1 << 11) >> 1);
	}

	{
		for (i = 0; i < 4; ++i)
		{
			uint16_t *probabilities = p->posSlotEncoder[i];
			uint32_t j;
			for (j = 0; j < (1 << 6); j++)
				probabilities[j] = ((1 << 11) >> 1);
		}
	}
	{
		for (i = 0; i < (1 << (14 >> 1)) - 14; ++i)
			p->posEncoders[i] = ((1 << 11) >> 1);
	}

	LenEnc_Init(&p->lenEnc.p);
	LenEnc_Init(&p->repLenEnc.p);

	for (i = 0; i < (1 << 4); ++i)
		p->posAlignEncoder[i] = ((1 << 11) >> 1);

	p->optimumEndIndex = 0;
	p->optimumCurrentIndex = 0;
	p->additionalOffset = 0;

	p->pb_mask = (1 << p->pb) - 1;
	p->lp_mask = (1 << p->lp) - 1;
}

void LzmaEnc_InitPrices(CLzmaEnc *p)
{
	if (!p->fastMode)
	{
		FillDistancesPrices(p);
		FillAlignPrices(p);
	}

	p->lenEnc.tableSize =
		p->repLenEnc.tableSize =
		p->numFastBytes + 1 - 2;
	LenPriceEnc_UpdateTables(&p->lenEnc, 1 << p->pb, p->ProbPrices);
	LenPriceEnc_UpdateTables(&p->repLenEnc, 1 << p->pb, p->ProbPrices);
}

static lzma_rc_t LzmaEnc_AllocAndInit(CLzmaEnc *p, uint32_t keepWindowSize, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	uint32_t i;
	for (i = 0; i < (uint32_t)(((9 + sizeof(size_t) / 2) - 1) * 2 + 7); ++i)
		if (p->dictionary_size <= ((uint32_t)1 << i))
			break;
	p->distTableSize = i * 2;

	p->finished = 0;
	p->result = LZMA_RC_OK;
	{ lzma_rc_t rc__ = (LzmaEnc_Alloc(p, keepWindowSize, allocator, allocator_large)); if (rc__ != 0) return rc__; };
	LzmaEnc_Init(p);
	LzmaEnc_InitPrices(p);
	p->nowPos64 = 0;
	return LZMA_RC_OK;
}

static lzma_rc_t LzmaEnc_Prepare(lzma_encoder_handle_t pp, lzma_seq_out_stream_t *out_stream, lzma_seq_in_stream_t *in_stream,
	lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	CLzmaEnc *p = (CLzmaEnc *)pp;
	p->matchFinderBase.stream = in_stream;
	p->needInit = 1;
	p->rc.out_stream = out_stream;
	return LzmaEnc_AllocAndInit(p, 0, allocator, allocator_large);
}

lzma_rc_t LzmaEnc_PrepareForLzma2(lzma_encoder_handle_t pp,
	lzma_seq_in_stream_t *in_stream, uint32_t keepWindowSize,
	lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	CLzmaEnc *p = (CLzmaEnc *)pp;
	p->matchFinderBase.stream = in_stream;
	p->needInit = 1;
	return LzmaEnc_AllocAndInit(p, keepWindowSize, allocator, allocator_large);
}

static void LzmaEnc_SetInputBuf(CLzmaEnc *p, const uint8_t *src, size_t src_len)
{
	p->matchFinderBase.directInput = 1;
	p->matchFinderBase.bufferBase = (uint8_t *)src;
	p->matchFinderBase.directInputRem = src_len;
}

lzma_rc_t LzmaEnc_MemPrepare(lzma_encoder_handle_t pp, const uint8_t *src, size_t src_len,
	uint32_t keepWindowSize, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	CLzmaEnc *p = (CLzmaEnc *)pp;
	LzmaEnc_SetInputBuf(p, src, src_len);
	p->needInit = 1;

	lzma_encoder_set_data_size(pp, src_len);
	return LzmaEnc_AllocAndInit(p, keepWindowSize, allocator, allocator_large);
}

void LzmaEnc_Finish(lzma_encoder_handle_t pp)
{

}


typedef struct
{
	lzma_seq_out_stream_t vtable;
	uint8_t *data;
	size_t rem;
	uint8_t overflow;
}
CLzmaEnc_SeqOutStreamBuf;


static size_t SeqOutStreamBuf_Write(const lzma_seq_out_stream_t *pp, const void *data, size_t size)
{
	CLzmaEnc_SeqOutStreamBuf *p = ((CLzmaEnc_SeqOutStreamBuf*)((uint8_t*)(1 ? (pp) : &((CLzmaEnc_SeqOutStreamBuf*)0)->vtable) - ((size_t)&(((CLzmaEnc_SeqOutStreamBuf *)0)->vtable))));

	if (p->rem < size)
	{
		size = p->rem;
		p->overflow = 1;
	}

	lzma_memcpy(p->data, data, size);

	p->rem -= size;
	p->data += size;
	return size;
}


uint32_t LzmaEnc_GetNumAvailableBytes(lzma_encoder_handle_t pp)
{
	const CLzmaEnc *p = (CLzmaEnc *)pp;
	return p->matchFinder.GetNumAvailableBytes(p->matchFinderObj);
}


const uint8_t *LzmaEnc_GetCurBuf(lzma_encoder_handle_t pp)
{
	const CLzmaEnc *p = (CLzmaEnc *)pp;
	return p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset;
}


lzma_rc_t LzmaEnc_CodeOneMemBlock(lzma_encoder_handle_t pp, uint8_t reInit,
	uint8_t *dest, size_t *dest_len, uint32_t desiredPackSize, uint32_t *unpacked_size)
{
	CLzmaEnc *p = (CLzmaEnc *)pp;
	uint64_t nowPos64;
	lzma_rc_t res;
	CLzmaEnc_SeqOutStreamBuf out_stream;

	out_stream.vtable.write = SeqOutStreamBuf_Write;
	out_stream.data = dest;
	out_stream.rem = *dest_len;
	out_stream.overflow = 0;

	p->write_end_mark = 0;
	p->finished = 0;
	p->result = LZMA_RC_OK;

	if (reInit)
		LzmaEnc_Init(p);
	LzmaEnc_InitPrices(p);
	nowPos64 = p->nowPos64;
	RangeEnc_Init(&p->rc);
	p->rc.out_stream = &out_stream.vtable;

	res = LzmaEnc_CodeOneBlock(p, 1, desiredPackSize, *unpacked_size);

	*unpacked_size = (uint32_t)(p->nowPos64 - nowPos64);
	*dest_len -= out_stream.rem;
	if (out_stream.overflow)
		return LZMA_RC_OUTPUT_EOF;

	return res;
}


static lzma_rc_t LzmaEnc_Encode2(CLzmaEnc *p, lzma_compress_progress_t *progress)
{
	lzma_rc_t res = LZMA_RC_OK;


	uint8_t allocaDummy[0x300];
	allocaDummy[0] = 0;
	allocaDummy[1] = allocaDummy[0];


	for (;;)
	{
		res = LzmaEnc_CodeOneBlock(p, 0, 0, 0);
		if (res != LZMA_RC_OK || p->finished)
			break;
		if (progress)
		{
			res = (progress)->progress(progress, p->nowPos64, ((&p->rc)->processed + ((&p->rc)->buffer - (&p->rc)->bufBase) + (&p->rc)->cacheSize));
			if (res != LZMA_RC_OK)
			{
				res = LZMA_RC_PROGRESS;
				break;
			}
		}
	}

	LzmaEnc_Finish(p);







	return res;
}


lzma_rc_t lzma_encoder_encode(lzma_encoder_handle_t pp, lzma_seq_out_stream_t *out_stream, lzma_seq_in_stream_t *in_stream, lzma_compress_progress_t *progress,
	lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	{ lzma_rc_t rc__ = (LzmaEnc_Prepare(pp, out_stream, in_stream, allocator, allocator_large)); if (rc__ != 0) return rc__; };
	return LzmaEnc_Encode2((CLzmaEnc *)pp, progress);
}


lzma_rc_t lzma_encoder_write_properties(lzma_encoder_handle_t pp, uint8_t *properties, size_t *size)
{
	CLzmaEnc *p = (CLzmaEnc *)pp;
	uint32_t i;
	uint32_t dictionary_size = p->dictionary_size;
	if (*size < 5)
		return LZMA_RC_PARAMETER;
	*size = 5;
	properties[0] = (uint8_t)((p->pb * 5 + p->lp) * 9 + p->lc);

	if (dictionary_size >= ((uint32_t)1 << 22))
	{
		uint32_t kDictMask = ((uint32_t)1 << 20) - 1;
		if (dictionary_size < UINT32_C(0xFFFFFFFF) - kDictMask)
			dictionary_size = (dictionary_size + kDictMask) & ~kDictMask;
	}
	else for (i = 11; i <= 30; ++i)
	{
		if (dictionary_size <= ((uint32_t)2 << i)) { dictionary_size = (2 << i); break; }
		if (dictionary_size <= ((uint32_t)3 << i)) { dictionary_size = (3 << i); break; }
	}

	for (i = 0; i < 4; ++i)
		properties[1 + i] = (uint8_t)(dictionary_size >> (8 * i));
	return LZMA_RC_OK;
}


uint32_t lzma_encoder_is_write_end_mark(lzma_encoder_handle_t pp)
{
	return ((CLzmaEnc *)pp)->write_end_mark;
}


lzma_rc_t lzma_encoder_encode_in_memory(lzma_encoder_handle_t pp, uint8_t *dest, size_t *dest_len, const uint8_t *src, size_t src_len,
	int32_t write_end_mark, lzma_compress_progress_t *progress, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	lzma_rc_t res;
	CLzmaEnc *p = (CLzmaEnc *)pp;

	CLzmaEnc_SeqOutStreamBuf out_stream;

	out_stream.vtable.write = SeqOutStreamBuf_Write;
	out_stream.data = dest;
	out_stream.rem = *dest_len;
	out_stream.overflow = 0;

	p->write_end_mark = write_end_mark;
	p->rc.out_stream = &out_stream.vtable;

	res = LzmaEnc_MemPrepare(pp, src, src_len, 0, allocator, allocator_large);

	if (res == LZMA_RC_OK)
	{
		res = LzmaEnc_Encode2(p, progress);

		if (res == LZMA_RC_OK && p->nowPos64 != src_len)
			res = LZMA_RC_FAILURE;
	}

	*dest_len -= out_stream.rem;

	if (out_stream.overflow)
		return LZMA_RC_OUTPUT_EOF;

	return res;
}


lzma_rc_t lzma_encode(uint8_t *dest, size_t *dest_len, const uint8_t *src, size_t src_len, const lzma_encoder_properties_t *properties, uint8_t *properties_encoded, size_t *properties_size, int32_t write_end_mark, lzma_compress_progress_t *progress, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	CLzmaEnc *p = (CLzmaEnc *)lzma_encoder_create(allocator);
	lzma_rc_t res;
	if (!p)
		return LZMA_RC_MEMORY;

	res = lzma_encoder_set_properties(p, properties);
	if (res == LZMA_RC_OK)
	{
		res = lzma_encoder_write_properties(p, properties_encoded, properties_size);
		if (res == LZMA_RC_OK)
			res = lzma_encoder_encode_in_memory(p, dest, dest_len, src, src_len,
				write_end_mark, progress, allocator, allocator_large);
	}

	lzma_encoder_destroy(p, allocator, allocator_large);

	return res;
}



























typedef struct
{
	lzma_seq_in_stream_t vtable;
	lzma_seq_in_stream_t *base_stream;
	uint64_t limit;
	uint64_t processed;
	int32_t finished;
} CLimitedSeqInStream;

static void LimitedSeqInStream_Init(CLimitedSeqInStream *p)
{
	p->limit = (uint64_t)(int64_t)-1;
	p->processed = 0;
	p->finished = 0;
}

static lzma_rc_t LimitedSeqInStream_Read(const lzma_seq_in_stream_t *pp, void *data, size_t *size)
{
	CLimitedSeqInStream *p = ((CLimitedSeqInStream*)((uint8_t*)(1 ? (pp) : &((CLimitedSeqInStream*)0)->vtable) - ((size_t)&(((CLimitedSeqInStream *)0)->vtable))));
	size_t size2 = *size;
	lzma_rc_t res = LZMA_RC_OK;

	if (p->limit != (uint64_t)(int64_t)-1)
	{
		uint64_t rem = p->limit - p->processed;
		if (size2 > rem)
			size2 = (size_t)rem;
	}
	if (size2 != 0)
	{
		res = (p->base_stream)->read(p->base_stream, data, &size2);
		p->finished = (size2 == 0 ? 1 : 0);
		p->processed += size2;
	}
	*size = size2;
	return res;
}




typedef struct
{
	lzma_encoder_handle_t enc;
	uint8_t propsAreSet;
	uint8_t propsByte;
	uint8_t need_to_init_state;
	uint8_t need_to_init_properties;
	uint64_t srcPos;
} CLzma2EncInt;


static lzma_rc_t Lzma2EncInt_InitStream(CLzma2EncInt *p, const lzma_2_encoder_properties_t *properties)
{
	if (!p->propsAreSet)
	{
		size_t properties_size = 5;
		uint8_t properties_encoded[5];
		{ lzma_rc_t rc__ = (lzma_encoder_set_properties(p->enc, &properties->lzmaProps)); if (rc__ != 0) return rc__; };
		{ lzma_rc_t rc__ = (lzma_encoder_write_properties(p->enc, properties_encoded, &properties_size)); if (rc__ != 0) return rc__; };
		p->propsByte = properties_encoded[0];
		p->propsAreSet = 1;
	}
	return LZMA_RC_OK;
}

static void Lzma2EncInt_InitBlock(CLzma2EncInt *p)
{
	p->srcPos = 0;
	p->need_to_init_state = 1;
	p->need_to_init_properties = 1;
}


lzma_rc_t LzmaEnc_PrepareForLzma2(lzma_encoder_handle_t pp, lzma_seq_in_stream_t *in_stream, uint32_t keepWindowSize,
	lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large);
lzma_rc_t LzmaEnc_MemPrepare(lzma_encoder_handle_t pp, const uint8_t *src, size_t src_len,
	uint32_t keepWindowSize, lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large);
lzma_rc_t LzmaEnc_CodeOneMemBlock(lzma_encoder_handle_t pp, uint8_t reInit,
	uint8_t *dest, size_t *dest_len, uint32_t desiredPackSize, uint32_t *unpacked_size);
const uint8_t *LzmaEnc_GetCurBuf(lzma_encoder_handle_t pp);
void LzmaEnc_Finish(lzma_encoder_handle_t pp);
void LzmaEnc_SaveState(lzma_encoder_handle_t pp);
void LzmaEnc_RestoreState(lzma_encoder_handle_t pp);





static lzma_rc_t Lzma2EncInt_EncodeSubblock(CLzma2EncInt *p, uint8_t *out_buffer,
	size_t *packSizeRes, lzma_seq_out_stream_t *out_stream)
{
	size_t packSizeLimit = *packSizeRes;
	size_t packed_size = packSizeLimit;
	uint32_t unpacked_size = (1 << 21);
	uint32_t lzHeaderSize = 5 + (p->need_to_init_properties ? 1 : 0);
	uint8_t useCopyBlock;
	lzma_rc_t res;

	*packSizeRes = 0;
	if (packed_size < lzHeaderSize)
		return LZMA_RC_OUTPUT_EOF;
	packed_size -= lzHeaderSize;

	LzmaEnc_SaveState(p->enc);
	res = LzmaEnc_CodeOneMemBlock(p->enc, p->need_to_init_state,
		out_buffer + lzHeaderSize, &packed_size, (1 << 16), &unpacked_size);

	;

	if (unpacked_size == 0)
		return res;

	if (res == LZMA_RC_OK)
		useCopyBlock = (packed_size + 2 >= unpacked_size || packed_size > (1 << 16));
	else
	{
		if (res != LZMA_RC_OUTPUT_EOF)
			return res;
		res = LZMA_RC_OK;
		useCopyBlock = 1;
	}

	if (useCopyBlock)
	{
		size_t destPos = 0;
		;

		while (unpacked_size > 0)
		{
			uint32_t u = (unpacked_size < (1 << 16)) ? unpacked_size : (1 << 16);
			if (packSizeLimit - destPos < u + 3)
				return LZMA_RC_OUTPUT_EOF;
			out_buffer[destPos++] = (uint8_t)(p->srcPos == 0 ? 1 : 2);
			out_buffer[destPos++] = (uint8_t)((u - 1) >> 8);
			out_buffer[destPos++] = (uint8_t)(u - 1);
			lzma_memcpy(out_buffer + destPos, LzmaEnc_GetCurBuf(p->enc) - unpacked_size, u);
			unpacked_size -= u;
			destPos += u;
			p->srcPos += u;

			if (out_stream)
			{
				*packSizeRes += destPos;
				if ((out_stream)->write(out_stream, out_buffer, destPos) != destPos)
					return LZMA_RC_WRITE;
				destPos = 0;
			}
			else
				*packSizeRes = destPos;

		}

		LzmaEnc_RestoreState(p->enc);
		return LZMA_RC_OK;
	}

	{
		size_t destPos = 0;
		uint32_t u = unpacked_size - 1;
		uint32_t pm = (uint32_t)(packed_size - 1);
		uint32_t mode = (p->srcPos == 0) ? 3 : (p->need_to_init_state ? (p->need_to_init_properties ? 2 : 1) : 0);

		;

		out_buffer[destPos++] = (uint8_t)((1 << 7) | (mode << 5) | ((u >> 16) & 0x1F));
		out_buffer[destPos++] = (uint8_t)(u >> 8);
		out_buffer[destPos++] = (uint8_t)u;
		out_buffer[destPos++] = (uint8_t)(pm >> 8);
		out_buffer[destPos++] = (uint8_t)pm;

		if (p->need_to_init_properties)
			out_buffer[destPos++] = p->propsByte;

		p->need_to_init_properties = 0;
		p->need_to_init_state = 0;
		destPos += packed_size;
		p->srcPos += unpacked_size;

		if (out_stream)
			if ((out_stream)->write(out_stream, out_buffer, destPos) != destPos)
				return LZMA_RC_WRITE;

		*packSizeRes = destPos;
		return LZMA_RC_OK;
	}
}




void lzma_2_encoder_properties_init(lzma_2_encoder_properties_t *p)
{
	lzma_encoder_properties_init(&p->lzmaProps);
	p->block_size = 0;
	p->block_threads_reduced = -1;
	p->block_threads_maximum = -1;
	p->total_threads = -1;
}

void lzma_2_encoder_properties_normalize(lzma_2_encoder_properties_t *p)
{
	uint64_t fileSize;
	int32_t t1, t1n, t2, t2r, t3;
	{
		lzma_encoder_properties_t lzmaProps = p->lzmaProps;
		lzma_encoder_properties_normalize(&lzmaProps);
		t1n = lzmaProps.thread_count;
	}

	t1 = p->lzmaProps.thread_count;
	t2 = p->block_threads_maximum;
	t3 = p->total_threads;

	if (t2 > 1)
		t2 = 1;

	if (t3 <= 0)
	{
		if (t2 <= 0)
			t2 = 1;
		t3 = t1n * t2;
	}
	else if (t2 <= 0)
	{
		t2 = t3 / t1n;
		if (t2 == 0)
		{
			t1 = 1;
			t2 = t3;
		}
		if (t2 > 1)
			t2 = 1;
	}
	else if (t1 <= 0)
	{
		t1 = t3 / t2;
		if (t1 == 0)
			t1 = 1;
	}
	else
		t3 = t1n * t2;

	p->lzmaProps.thread_count = t1;

	t2r = t2;

	fileSize = p->lzmaProps.reduce_size;

	if (p->block_size != ((uint64_t)(int64_t)-1)
		&& p->block_size != 0
		&& (p->block_size < fileSize || fileSize == (uint64_t)(int64_t)-1))
		p->lzmaProps.reduce_size = p->block_size;

	lzma_encoder_properties_normalize(&p->lzmaProps);

	p->lzmaProps.reduce_size = fileSize;

	t1 = p->lzmaProps.thread_count;

	if (p->block_size == ((uint64_t)(int64_t)-1))
	{
		t2r = t2 = 1;
		t3 = t1;
	}
	else if (p->block_size == 0 && t2 <= 1)
	{

		p->block_size = ((uint64_t)(int64_t)-1);
	}
	else
	{
		if (p->block_size == 0)
		{
			const uint32_t kMinSize = (uint32_t)1 << 20;
			const uint32_t kMaxSize = (uint32_t)1 << 28;
			const uint32_t dictionary_size = p->lzmaProps.dictionary_size;
			uint64_t block_size = (uint64_t)dictionary_size << 2;
			if (block_size < kMinSize) block_size = kMinSize;
			if (block_size > kMaxSize) block_size = kMaxSize;
			if (block_size < dictionary_size) block_size = dictionary_size;
			block_size += (kMinSize - 1);
			block_size &= ~(uint64_t)(kMinSize - 1);
			p->block_size = block_size;
		}

		if (t2 > 1 && fileSize != (uint64_t)(int64_t)-1)
		{
			uint64_t numBlocks = fileSize / p->block_size;
			if (numBlocks * p->block_size != fileSize)
				numBlocks++;
			if (numBlocks < (uint32_t)t2)
			{
				t2r = (uint32_t)numBlocks;
				if (t2r == 0)
					t2r = 1;
				t3 = t1 * t2r;
			}
		}
	}

	p->block_threads_maximum = t2;
	p->block_threads_reduced = t2r;
	p->total_threads = t3;
}


static lzma_rc_t progress(lzma_compress_progress_t *p, uint64_t in_size, uint64_t out_size)
{
	return (p && (p)->progress(p, in_size, out_size) != LZMA_RC_OK) ? LZMA_RC_PROGRESS : LZMA_RC_OK;
}


typedef struct
{
	uint8_t propEncoded;
	lzma_2_encoder_properties_t properties;
	uint64_t expectedDataSize;

	uint8_t *tempBufLzma;

	lzma_allocator_ptr_t allocator;
	lzma_allocator_ptr_t allocator_large;

	CLzma2EncInt coders[1];
}
CLzma2Enc;


lzma_2_encoder_handle_t lzma_2_encoder_create(lzma_allocator_ptr_t allocator, lzma_allocator_ptr_t allocator_large)
{
	CLzma2Enc *p = (CLzma2Enc *)(allocator)->allocate(allocator, sizeof(CLzma2Enc));

	if (!p) return ((void *)0);

	lzma_2_encoder_properties_init(&p->properties);
	lzma_2_encoder_properties_normalize(&p->properties);
	p->expectedDataSize = (uint64_t)(int64_t)-1;
	p->tempBufLzma = ((void *)0);
	p->allocator = allocator;
	p->allocator_large = allocator_large;

	{
		uint32_t i;
		for (i = 0; i < 1; ++i)
			p->coders[i].enc = ((void *)0);
	}

	return p;
}


void lzma_2_encoder_destroy(lzma_2_encoder_handle_t pp)
{
	CLzma2Enc *p = (CLzma2Enc *)pp;
	uint32_t i;

	for (i = 0; i < 1; ++i)
	{
		CLzma2EncInt *t = &p->coders[i];
		if (t->enc)
		{
			lzma_encoder_destroy(t->enc, p->allocator, p->allocator_large);
			t->enc = ((void *)0);
		}
	}

	(p->allocator)->release(p->allocator, p->tempBufLzma);
	p->tempBufLzma = ((void *)0);

	(p->allocator)->release(p->allocator, pp);
}


lzma_rc_t lzma_2_encoder_set_properties(lzma_2_encoder_handle_t pp, const lzma_2_encoder_properties_t *properties)
{
	CLzma2Enc *p = (CLzma2Enc *)pp;
	lzma_encoder_properties_t lzmaProps = properties->lzmaProps;
	lzma_encoder_properties_normalize(&lzmaProps);

	if (lzmaProps.lc + lzmaProps.lp > 4)
		return LZMA_RC_PARAMETER;

	p->properties = *properties;
	lzma_2_encoder_properties_normalize(&p->properties);
	return LZMA_RC_OK;
}


void lzma_2_encoder_set_data_size(lzma_encoder_handle_t pp, uint64_t expected_data_size)
{
	CLzma2Enc *p = (CLzma2Enc *)pp;
	p->expectedDataSize = expected_data_size;
}


uint8_t lzma_2_encoder_write_properties(lzma_2_encoder_handle_t pp)
{
	CLzma2Enc *p = (CLzma2Enc *)pp;
	uint32_t i;
	uint32_t dictionary_size = lzma_encoder_properties_dictionary_size(&p->properties.lzmaProps);

	for (i = 0; i < 40; ++i)
		if (dictionary_size <= (((uint32_t)2 | ((i) & 1)) << ((i) / 2 + 11)))
			break;

	return (uint8_t)i;
}


static lzma_rc_t Lzma2Enc_EncodeMt1(CLzma2Enc *me, CLzma2EncInt *p, lzma_seq_out_stream_t *out_stream, uint8_t *out_buffer, size_t *out_buffer_size, lzma_seq_in_stream_t *in_stream, const uint8_t *in_data, size_t in_data_size, int32_t finished, lzma_compress_progress_t *progress)
{
	uint64_t unpackTotal = 0;
	uint64_t packTotal = 0;
	size_t outLim = 0;
	CLimitedSeqInStream limitedInStream;

	if (out_buffer)
	{
		outLim = *out_buffer_size;
		*out_buffer_size = 0;
	}

	if (!p->enc)
	{
		p->propsAreSet = 0;
		p->enc = lzma_encoder_create(me->allocator);

		if (!p->enc)
			return LZMA_RC_MEMORY;
	}

	limitedInStream.base_stream = in_stream;

	if (in_stream)
		limitedInStream.vtable.read = LimitedSeqInStream_Read;

	if (!out_buffer)
	{

		if (!me->tempBufLzma)
		{
			me->tempBufLzma = (uint8_t *)(me->allocator)->allocate(me->allocator, ((1 << 16) + 16));

			if (!me->tempBufLzma)
				return LZMA_RC_MEMORY;
		}
	}

	{ lzma_rc_t rc__ = (Lzma2EncInt_InitStream(p, &me->properties)); if (rc__ != 0) return rc__; };

	for (;;)
	{
		lzma_rc_t res = LZMA_RC_OK;
		size_t inSizeCur = 0;

		Lzma2EncInt_InitBlock(p);

		LimitedSeqInStream_Init(&limitedInStream);
		limitedInStream.limit = me->properties.block_size;

		if (in_stream)
		{
			uint64_t expected = (uint64_t)(int64_t)-1;

			if (me->expectedDataSize != (uint64_t)(int64_t)-1
				&& me->expectedDataSize >= unpackTotal)
				expected = me->expectedDataSize - unpackTotal;
			if (me->properties.block_size != ((uint64_t)(int64_t)-1)
				&& expected > me->properties.block_size)
				expected = (size_t)me->properties.block_size;

			lzma_encoder_set_data_size(p->enc, expected);

			{ lzma_rc_t rc__ = (LzmaEnc_PrepareForLzma2(p->enc, &limitedInStream.vtable, (1 << 21), me->allocator, me->allocator_large)); if (rc__ != 0) return rc__; };
		}
		else
		{
			inSizeCur = in_data_size - (size_t)unpackTotal;
			if (me->properties.block_size != ((uint64_t)(int64_t)-1)
				&& inSizeCur > me->properties.block_size)
				inSizeCur = (size_t)me->properties.block_size;



			{ lzma_rc_t rc__ = (LzmaEnc_MemPrepare(p->enc, in_data + (size_t)unpackTotal, inSizeCur, (1 << 21), me->allocator, me->allocator_large)); if (rc__ != 0) return rc__; };
		}

		for (;;)
		{
			size_t packed_size = ((1 << 16) + 16);
			if (out_buffer)
				packed_size = outLim - (size_t)packTotal;

			res = Lzma2EncInt_EncodeSubblock(p,
				out_buffer ? out_buffer + (size_t)packTotal : me->tempBufLzma, &packed_size,
				out_buffer ? ((void *)0) : out_stream);

			if (res != LZMA_RC_OK)
				break;

			packTotal += packed_size;

			if (out_buffer)
				*out_buffer_size = (size_t)packTotal;

			res = progress->progress(progress, unpackTotal + p->srcPos, packTotal);

			if (res != LZMA_RC_OK)
				break;

			if (packed_size == 0)
				break;
		}

		LzmaEnc_Finish(p->enc);

		unpackTotal += p->srcPos;

		{ lzma_rc_t rc__ = (res); if (rc__ != 0) return rc__; };

		if (p->srcPos != (in_stream ? limitedInStream.processed : inSizeCur))
			return LZMA_RC_FAILURE;

		if (in_stream ? limitedInStream.finished : (unpackTotal == in_data_size))
		{
			if (finished)
			{
				if (out_buffer)
				{
					size_t destPos = *out_buffer_size;
					if (destPos >= outLim)
						return LZMA_RC_OUTPUT_EOF;
					out_buffer[destPos] = 0;
					*out_buffer_size = destPos + 1;
				}
				else
				{
					uint8_t b = 0;
					if ((out_stream)->write(out_stream, &b, 1) != 1)
						return LZMA_RC_WRITE;
				}
			}
			return LZMA_RC_OK;
		}
	}
}


lzma_rc_t lzma_2_encoder_encode_ex(lzma_2_encoder_handle_t pp, lzma_seq_out_stream_t *out_stream, uint8_t *out_buffer, size_t *out_buffer_size, lzma_seq_in_stream_t *in_stream, const uint8_t *in_data, size_t in_data_size, lzma_compress_progress_t *progress)
{
	CLzma2Enc* p = (CLzma2Enc*)pp;

	if (in_stream && in_data)
		return LZMA_RC_PARAMETER;

	if (out_stream && out_buffer)
		return LZMA_RC_PARAMETER;

	p->coders[0].propsAreSet = 0;

	return Lzma2Enc_EncodeMt1(p, &p->coders[0], out_stream, out_buffer, out_buffer_size, in_stream, in_data, in_data_size, 1, progress);
}


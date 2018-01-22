// lzma.c - LZMA compression support.


#include <stdint.h>

#include "../config.h"
#include "lzma.h"


typedef struct lzma_seq_in_strm_buf_s
{
	uint8_t* buffer;
	size_t buffer_size;
	size_t position;
}
lzma_seq_in_strm_buf_t;


typedef struct lzma_enc_seq_out_strm_buf_s
{
	uint8_t* data;
	size_t rem;
	uint8_t overflow;
}
lzma_seq_out_strm_buf_t;


typedef void* lzma_encoder_handle_t;


inline static void lzma_encoder_properties_init(lzma_encoder_properties_t* properties);
inline static void lzma_encoder_properties_normalize(lzma_encoder_properties_t* properties);
inline static uint32_t lzma_encoder_properties_dictionary_size(lzma_encoder_properties_t* properties);
inline static lzma_encoder_handle_t lzma_encoder_create(lzma_allocator_ptr_t allocator);
inline static void lzma_encoder_destroy(lzma_encoder_handle_t p, lzma_allocator_ptr_t allocator);
inline static lzma_rc_t lzma_encoder_set_properties(lzma_encoder_handle_t p, const lzma_encoder_properties_t* properties);
inline static void lzma_encoder_set_data_size(lzma_encoder_handle_t p, uint64_t expected_data_size);
inline static lzma_rc_t lzma_encoder_write_properties(lzma_encoder_handle_t p, uint8_t* properties, size_t* size);
inline static uint8_t lzma_encoder_is_write_end_mark(lzma_encoder_handle_t p);
inline static lzma_rc_t lzma_encoder_encode(lzma_encoder_handle_t p, lzma_seq_out_strm_buf_t* out_stream, lzma_seq_in_strm_buf_t* in_stream, lzma_allocator_ptr_t allocator);
inline static lzma_rc_t lzma_encoder_encode_in_memory(lzma_encoder_handle_t p, uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t src_len, uint8_t write_end_mark, lzma_allocator_ptr_t allocator);
inline static size_t lzma_seq_out_strm_buf_write(lzma_seq_out_strm_buf_t* p, const void *data, size_t size);
inline static lzma_rc_t lzma_seq_in_strm_buf_read(lzma_seq_in_strm_buf_t* p, uint8_t* dest, size_t* size);
inline static lzma_rc_t lzma_properties_decode(lzma_properties_t* p, const uint8_t* data, uint32_t size);


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


inline static void lzma_decoder_init(lzma_decoder_t* p);
inline static lzma_rc_t lzma_decoder_allocate_probabilities(lzma_decoder_t* p, const uint8_t* properties, uint32_t properties_size, lzma_allocator_ptr_t allocator);
inline static void lzma_decoder_release_probabilities(lzma_decoder_t* p, lzma_allocator_ptr_t allocator);
inline static lzma_rc_t lzma_decoder_allocate(lzma_decoder_t* state, const uint8_t* properties, uint32_t properties_size, lzma_allocator_ptr_t allocator);
inline static void lzma_decoder_release(lzma_decoder_t* state, lzma_allocator_ptr_t allocator);
inline static lzma_rc_t lzma_decoder_decode_to_dictionary(lzma_decoder_t* p, size_t dictionary_limit, const uint8_t* src, size_t* src_len, lzma_finish_mode_t finish_mode, lzma_status_t* status);
inline static lzma_rc_t lzma_decoder_decode_to_buffer(lzma_decoder_t* p, uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t* src_len, lzma_finish_mode_t finish_mode, lzma_status_t* status);


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


inline static int32_t lzma_decoder_decode_real(lzma_decoder_t* p, size_t limit, const uint8_t* buffer_limit)
{
	uint16_t* probabilities = p->probabilities;
	uint32_t state = p->state;
	uint32_t rep_a = p->reps[0], rep_b = p->reps[1], rep_c = p->reps[2], rep_d = p->reps[3];
	uint32_t pb_mask = (UINT32_C(1) << (p->properties.pb)) - UINT32_C(1);
	uint32_t lp_mask = (UINT32_C(1) << (p->properties.lp)) - UINT32_C(1);
	uint32_t lc = p->properties.lc;
	uint8_t* dictionary = p->dictionary;
	size_t dictionary_buffer_size = p->dictionary_buffer_size;
	size_t dictionary_position = p->dictionary_position;
	uint32_t processed_position = p->processed_position;
	uint32_t check_dictionary_size = p->check_dictionary_size;
	uint32_t len = UINT32_C(0);
	const uint8_t* buffer = p->buffer;
	uint32_t range = p->range;
	uint32_t code = p->code;

	do
	{
		uint16_t* prob;
		uint32_t bound;
		uint32_t temp_a;
		uint32_t pos_state = processed_position & pb_mask;

		prob = probabilities + (state << 4) + pos_state;

		temp_a = *prob;

		if (range < UINT32_C(0x1000000))
		{
			range <<= 8;
			code = (code << 8) | (*buffer++);
		}

		bound = (range >> 11) * temp_a;

		if (code < bound)
		{
			uint32_t symbol;
			range = bound; *prob = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));

			prob = probabilities + UINT32_C(0x736);

			if (processed_position || check_dictionary_size)
				prob += (UINT32_C(0x300) * (((processed_position & lp_mask) << lc) +
				(dictionary[(!dictionary_position ? dictionary_buffer_size : dictionary_position) - 1] >> (UINT32_C(8) - lc))));

			processed_position++;

			if (state < 7)
			{
				state -= (state < UINT32_C(4)) ? state : UINT32_C(3);
				symbol = UINT32_C(1);

				do
				{
					temp_a = *(prob + symbol);

					if (range < UINT32_C(0x1000000))
					{
						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

					bound = (range >> 11) * temp_a;

					if (code < bound)
					{
						range = bound;
						*(prob + symbol) = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
						symbol += symbol;
					}
					else
					{
						range -= bound;
						code -= bound;
						*(prob + symbol) = (uint16_t)(temp_a - (temp_a >> 5));
						symbol += symbol + UINT32_C(1);
					}
				} 
				while (symbol < UINT32_C(0x100));
			}
			else
			{
				uint32_t match_byte = dictionary[dictionary_position - rep_a + (dictionary_position < rep_a) ? dictionary_buffer_size : UINT32_C(0)];
				uint32_t offs = UINT32_C(0x100);

				state -= (state < UINT32_C(10)) ? UINT32_C(3) : UINT32_C(6);
				symbol = UINT32_C(1);

				do
				{
					uint32_t bit;
					uint16_t* prob_lit;

					match_byte <<= 1;
					bit = (match_byte & offs);
					prob_lit = prob + offs + bit + symbol;
					temp_a = *prob_lit;

					if (range < UINT32_C(0x1000000))
					{
						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

					bound = (range >> 11) * temp_a;

					if (code < bound)
					{
						range = bound;
						*prob_lit = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
						symbol = (symbol + symbol);
						offs &= ~bit;
					}
					else
					{
						range -= bound;
						code -= bound;
						*prob_lit = (uint16_t)(temp_a - (temp_a >> 5));
						symbol = (symbol + symbol) + UINT32_C(1);
						offs &= bit;
					}
				} 
				while (symbol < UINT32_C(0x100));
			}

			dictionary[dictionary_position++] = (uint8_t)symbol;

			continue;
		}

		range -= bound;
		code -= bound;
		*prob = (uint16_t)(temp_a - (temp_a >> 5));
		prob = probabilities + UINT32_C(0xC0) + state;
		temp_a = *prob;

		if (range < UINT32_C(0x1000000))
		{
			range <<= 8;
			code = (code << 8) | (*buffer++);
		}

		bound = (range >> 11) * temp_a;

		if (code < bound)
		{
			range = bound;
			*prob = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
			state += UINT32_C(12);
			prob = probabilities + UINT32_C(0x342);
		}
		else
		{
			range -= bound;
			code -= bound;
			*prob = (uint16_t)(temp_a - (temp_a >> 5));

			if (!check_dictionary_size && !processed_position)
				return LZMA_RC_DATA;

			prob = probabilities + UINT32_C(0xCC) + state;

			temp_a = *prob;

			if (range < UINT32_C(0x1000000))
			{
				range <<= 8;
				code = (code << 8) | (*buffer++);
			}

			bound = (range >> 11) * temp_a;

			if (code < bound)
			{
				range = bound;
				*prob = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
				prob = probabilities + UINT32_C(0xF0) + (state << 4) + pos_state;

				temp_a = *prob;

				if (range < UINT32_C(0x1000000))
				{
					range <<= 8;
					code = (code << 8) | (*buffer++);
				}

				bound = (range >> 11) * temp_a;

				if (code < bound)
				{
					range = bound;

					*prob = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));

					dictionary[dictionary_position] = dictionary[dictionary_position - rep_a + (dictionary_position < rep_a ? dictionary_buffer_size : 0)];
					dictionary_position++;
					processed_position++;

					state = (state < UINT32_C(7)) ? UINT32_C(9) : UINT32_C(11);

					continue;
				}

				range -= bound;
				code -= bound;
				*prob = (uint16_t)(temp_a - (temp_a >> 5));
			}
			else
			{
				uint32_t distance;

				range -= bound;
				code -= bound;
				*prob = (uint16_t)(temp_a - (temp_a >> 5));
				prob = probabilities + UINT32_C(0xD8) + state;
				temp_a = *prob;

				if (range < UINT32_C(0x1000000))
				{
					range <<= 8;
					code = (code << 8) | (*buffer++);
				}

				bound = (range >> 11) * temp_a;

				if (code < bound)
				{
					range = bound;
					*prob = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
					distance = rep_b;
				}
				else
				{
					range -= bound;
					code -= bound;
					*prob = (uint16_t)(temp_a - (temp_a >> 5));
					prob = probabilities + UINT32_C(0xE4) + state;
					temp_a = *prob;

					if (range < UINT32_C(0x1000000))
					{
						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

					bound = (range >> 11) * temp_a;

					if (code < bound)
					{
						range = bound;
						*prob = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
						distance = rep_c;
					}
					else
					{
						range -= bound;
						code -= bound;
						*prob = (uint16_t)(temp_a - (temp_a >> 5));
						distance = rep_d;
						rep_d = rep_c;
					}

					rep_c = rep_b;
				}

				rep_b = rep_a;
				rep_a = distance;
			}

			state = (state < UINT32_C(7)) ? UINT32_C(8) : UINT32_C(11);
			prob = probabilities + UINT32_C(0x544);
		}

		uint32_t lim, offset;
		uint16_t* prob_len = prob;

		temp_a = *prob_len;

		if (range < UINT32_C(0x1000000))
		{
			range <<= 8;
			code = (code << 8) | (*buffer++);
		}

		bound = (range >> 11) * temp_a;

		if (code < bound)
		{
			range = bound;
			*prob_len = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
			prob_len = prob + UINT32_C(2) + (pos_state << 3);
			offset = 0;
			lim = UINT32_C(8);
		}
		else
		{
			range -= bound;
			code -= bound;
			*prob_len = (uint16_t)(temp_a - (temp_a >> 5));
			prob_len = prob + UINT32_C(1);
			temp_a = *prob_len;

			if (range < UINT32_C(0x1000000))
			{
				range <<= 8;
				code = (code << 8) | (*buffer++);
			}

			bound = (range >> 11) * temp_a;

			if (code < bound)
			{
				range = bound;
				*prob_len = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
				prob_len = prob + UINT32_C(0x82) + (pos_state << 3);
				offset = UINT32_C(8);
				lim = UINT32_C(8);
			}
			else
			{
				range -= bound;
				code -= bound;
				*prob_len = (uint16_t)(temp_a - (temp_a >> 5));
				prob_len = prob + UINT32_C(0x102);
				offset = UINT32_C(0x10);
				lim = UINT32_C(0x100);
			}
		}

		len = UINT32_C(1);

		do
		{
			temp_a = *(prob_len + len);

			if (range < UINT32_C(0x1000000))
			{
				range <<= 8;
				code = (code << 8) | (*buffer++);
			}

			bound = (range >> 11) * temp_a;

			if (code < bound)
			{
				range = bound;
				*(prob_len + len) = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
				len += len;
			}
			else
			{
				range -= bound;
				code -= bound;
				*(prob_len + len) = (uint16_t)(temp_a - (temp_a >> 5));
				len += len + UINT32_C(1);
			}
		} 
		while (len < lim);

		len -= lim;
		len += offset;

		if (state >= UINT32_C(12))
		{
			uint32_t distance = UINT32_C(1);

			prob = probabilities + UINT32_C(0x1B0) + ((len < UINT32_C(4) ? len : UINT32_C(3)) << 6);

			do
			{
				temp_a = *(prob + distance);

				if (range < UINT32_C(0x1000000))
				{
					range <<= 8;
					code = (code << 8) | (*buffer++);
				}

				bound = (range >> 11) * temp_a;

				if (code < bound)
				{
					range = bound;
					*(prob + distance) = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
					distance = (distance + distance);
				}
				else
				{
					range -= bound;
					code -= bound;
					*(prob + distance) = (uint16_t)(temp_a - (temp_a >> 5));
					distance = (distance + distance) + UINT32_C(1);
				}
			} 
			while (distance < UINT32_C(0x40));

			distance -= UINT32_C(0x40);

			if (distance >= UINT32_C(4))
			{
				uint32_t pos_slot = (uint32_t)distance;
				uint32_t nr_direct_bits = (uint32_t)((distance >> 1) - UINT32_C(1));

				distance = (UINT32_C(2) | (distance & UINT32_C(1)));

				if (pos_slot < UINT32_C(14))
				{
					distance <<= nr_direct_bits;
					prob = probabilities + UINT32_C(0x2B0) + distance - pos_slot - UINT32_C(1);

					uint32_t i = UINT32_C(0), mask = UINT32_C(1);

					do
					{
						temp_a = *(prob + i);

						if (range < UINT32_C(0x1000000))
						{
							range <<= 8;
							code = (code << 8) | (*buffer++);
						}

						bound = (range >> 11) * temp_a;

						if (code < bound)
						{
							range = bound;
							*(prob + i) = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
							i += i;
						}
						else
						{
							range -= bound;
							code -= bound;
							*(prob + i) = (uint16_t)(temp_a - (temp_a >> 5));
							i += i + UINT32_C(1);
							distance |= mask;
						}

						mask <<= 1;
					} 
					while (--nr_direct_bits);
				}
				else
				{
					nr_direct_bits -= UINT32_C(4);

					do
					{
						if (range < UINT32_C(0x1000000))
						{
							range <<= 8;
							code = (code << 8) | (*buffer++);
						}

						range >>= 1;

						uint32_t t;
						code -= range;
						t = UINT32_C(0) - (code >> 31);
						distance = (distance << 1) + t + UINT32_C(1);
						code += range & t;
					} 
					while (--nr_direct_bits);

					prob = probabilities + UINT32_C(0x322);
					distance <<= 4;

					uint32_t i = UINT32_C(1);

					temp_a = *(prob + i);

					if (range < UINT32_C(0x1000000))
					{
						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

					bound = (range >> 11) * temp_a;

					if (code < bound)
					{
						range = bound;
						*(prob + i) = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
						i += i;
					}
					else
					{
						range -= bound;
						code -= bound;
						*(prob + i) = (uint16_t)(temp_a - (temp_a >> 5));
						i += i + UINT32_C(1);
						distance |= UINT32_C(1);
					}

					temp_a = *(prob + i);

					if (range < UINT32_C(0x1000000))
					{
						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

					bound = (range >> 11) * temp_a;

					if (code < bound)
					{
						range = bound;
						*(prob + i) = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
						i = (i + i);
					}
					else
					{
						range -= bound;
						code -= bound;
						*(prob + i) = (uint16_t)(temp_a - (temp_a >> 5));
						i += i + UINT32_C(1);
						distance |= UINT32_C(2);
					}

					temp_a = *(prob + i);

					if (range < UINT32_C(0x1000000))
					{
						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

					bound = (range >> 11) * temp_a;

					if (code < bound)
					{
						range = bound;
						*(prob + i) = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
						i += i;
					}
					else
					{
						range -= bound;
						code -= bound; *(prob + i) = (uint16_t)(temp_a - (temp_a >> 5));
						i += i + UINT32_C(1);
						distance |= UINT32_C(4);
					}

					temp_a = *(prob + i);

					if (range < UINT32_C(0x1000000))
					{
						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

					bound = (range >> 11) * temp_a;

					if (code < bound)
					{
						range = bound;
						*(prob + i) = (uint16_t)(temp_a + ((UINT32_C(0x800) - temp_a) >> 5));
						i += i;
					}
					else
					{
						range -= bound;
						code -= bound;
						*(prob + i) = (uint16_t)(temp_a - (temp_a >> 5));
						i += i + UINT32_C(1);
						distance |= UINT32_C(8);
					}

					if (distance == UINT32_C(0xFFFFFFFF))
					{
						len += UINT32_C(0x112);
						state -= UINT32_C(12);
						break;
					}
				}
			}

			rep_d = rep_c;
			rep_c = rep_b;
			rep_b = rep_a;
			rep_a = distance + UINT32_C(1);

			if (!check_dictionary_size)
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

			state = (state < UINT32_C(19)) ? UINT32_C(7) : UINT32_C(10);
		}

		len += UINT32_C(2);

		size_t rem;
		uint32_t curr_len;
		size_t pos;

		if (!(rem = limit - dictionary_position))
		{
			p->dictionary_position = dictionary_position;

			return LZMA_RC_DATA;
		}

		curr_len = (rem < len) ? (uint32_t)rem : len;
		pos = dictionary_position - rep_a + (dictionary_position < (size_t)rep_a) ? dictionary_buffer_size : UINT64_C(0);

		processed_position += curr_len;

		len -= curr_len;

		if (curr_len <= dictionary_buffer_size - pos)
		{
			uint8_t* dest = dictionary + dictionary_position;
			ptrdiff_t src = (ptrdiff_t)pos - (ptrdiff_t)dictionary_position;
			const uint8_t* lim = dest + curr_len;

			dictionary_position += curr_len;

			do *dest = (uint8_t)*(dest + src);
			while (++dest != lim);
		}
		else
		{
			do
			{
				dictionary[dictionary_position++] = dictionary[pos];

				if (++pos == dictionary_buffer_size)
					pos = UINT64_C(0);
			} 
			while (--curr_len);
		}
	} 
	while (dictionary_position < limit && buffer < buffer_limit);

	if (range < UINT32_C(0x1000000))
	{
		range <<= 8;
		code = (code << 8) | (*buffer++);
	}

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


inline static void  lzma_decoder_write_rem(lzma_decoder_t* p, size_t limit)
{
	if (p->remaining_length != UINT32_C(0) && p->remaining_length < UINT32_C(0x112))
	{
		uint8_t *dictionary = p->dictionary;
		size_t dictionary_position = p->dictionary_position;
		size_t dictionary_buffer_size = p->dictionary_buffer_size;
		uint32_t len = p->remaining_length;
		size_t rep_a = p->reps[0];
		size_t rem = limit - dictionary_position;

		if (rem < len)
			len = (uint32_t)(rem);

		if (!p->check_dictionary_size && p->properties.dictionary_size - p->processed_position <= len)
			p->check_dictionary_size = p->properties.dictionary_size;

		p->processed_position += len;
		p->remaining_length -= len;

		while (len)
		{
			len--;
			dictionary[dictionary_position] = dictionary[dictionary_position - rep_a + (dictionary_position < rep_a ? dictionary_buffer_size : 0)];
			dictionary_position++;
		}

		p->dictionary_position = dictionary_position;
	}
}


inline static lzma_rc_t lzma_decoder_decode_real_ex(lzma_decoder_t* p, size_t limit, const uint8_t *buffer_limit)
{
	lzma_rc_t rc;

	do
	{
		size_t limit_b = limit;

		if (!p->check_dictionary_size)
		{
			uint32_t rem = p->properties.dictionary_size - p->processed_position;

			if (limit - p->dictionary_position > rem)
				limit_b = p->dictionary_position + rem;
		}

		rc = lzma_decoder_decode_real(p, limit_b, buffer_limit); 
		
		if (rc) return rc;

		if (!p->check_dictionary_size && p->processed_position >= p->properties.dictionary_size)
			p->check_dictionary_size = p->properties.dictionary_size;

		lzma_decoder_write_rem(p, limit);
	} 
	while (p->dictionary_position < limit && p->buffer < buffer_limit && p->remaining_length < UINT32_C(0x112));

	if (p->remaining_length > UINT32_C(0x112))
		p->remaining_length = UINT32_C(0x112);

	return LZMA_RC_OK;
}


typedef enum lzma_dummy_e
{
	LZMA_DUMMY_ERROR,
	LZMA_DUMMY_LIT,
	LZMA_DUMMY_MATCH,
	LZMA_DUMMY_REP
} 
lzma_dummy_t;


inline static lzma_dummy_t lzma_decoder_try_dummy(const lzma_decoder_t* p, const uint8_t* buffer, size_t in_size)
{
	lzma_dummy_t res;
	const uint16_t* prob;
	uint32_t range = p->range, code = p->code;
	const uint8_t* buffer_limit = buffer + in_size;
	const uint16_t* probabilities = p->probabilities;
	uint32_t bound, temp_a, state = p->state, pos_state = (p->processed_position) & ((UINT32_C(1) << p->properties.pb) - UINT32_C(1));

	prob = probabilities + (state << 4) + pos_state;

	temp_a = *prob;

	if (range < UINT32_C(0x1000000))
	{
		if (buffer >= buffer_limit) 
			return LZMA_DUMMY_ERROR;

		range <<= 8;
		code = (code << 8) | (*buffer++);
	}

	bound = (range >> 11) * temp_a;

	if (code < bound)
	{
		range = bound;

		prob = probabilities + UINT32_C(0x736);

		if (p->check_dictionary_size || p->processed_position)
			prob += (UINT32_C(0x300) * ((((p->processed_position) & ((UINT32_C(1) << (p->properties.lp)) - UINT32_C(1))) << p->properties.lc) +
			(p->dictionary[((!p->dictionary_position) ? p->dictionary_buffer_size : p->dictionary_position) - UINT64_C(1)] >> (UINT32_C(8) - p->properties.lc))));

		if (state < UINT32_C(7))
		{
			uint32_t symbol = UINT32_C(1);

			do
			{
				temp_a = *(prob + symbol);

				if (range < UINT32_C(0x1000000))
				{
					if (buffer >= buffer_limit) 
						return LZMA_DUMMY_ERROR;

					range <<= 8;
					code = (code << 8) | (*buffer++);
				}

				bound = (range >> 11) * temp_a;

				if (code < bound)
				{
					range = bound;
					symbol = (symbol + symbol);
				}
				else
				{
					range -= bound;
					code -= bound;
					symbol += symbol + UINT32_C(1);
				}
			} 
			while (symbol < 0x100);
		}
		else
		{
			uint32_t match_byte = p->dictionary[p->dictionary_position - p->reps[0] + ((p->dictionary_position < p->reps[0]) ? p->dictionary_buffer_size : UINT32_C(0))];
			uint32_t offs = UINT32_C(0x100);
			uint32_t symbol = UINT32_C(1);

			do
			{
				match_byte <<= 1;

				uint32_t bit = (match_byte & offs);
				const uint16_t* prob_lit = prob + offs + bit + symbol;

				temp_a = *prob_lit;

				if (range < UINT32_C(0x1000000))
				{
					if (buffer >= buffer_limit)
						return LZMA_DUMMY_ERROR;

					range <<= 8;
					code = (code << 8) | (*buffer++);
				}

				bound = (range >> 11) * temp_a;

				if (code < bound)
				{
					range = bound;
					symbol = (symbol + symbol);
					offs &= ~bit;
				}
				else
				{
					range -= bound;
					code -= bound;
					symbol += symbol + UINT32_C(1);
					offs &= bit;
				}
			}
			while (symbol < UINT32_C(0x100));
		}

		res = LZMA_DUMMY_LIT;
	}
	else
	{
		uint32_t len;

		range -= bound; code -= bound;
		prob = probabilities + UINT32_C(0xC0) + state;
		temp_a = *prob;

		if (range < UINT32_C(0x1000000))
		{
			if (buffer >= buffer_limit)
				return LZMA_DUMMY_ERROR;

			range <<= 8;
			code = (code << 8) | (*buffer++);
		}

		bound = (range >> 11) * temp_a;

		if (code < bound)
		{
			range = bound;
			state = UINT32_C(0);
			prob = probabilities + UINT32_C(0x342);
			res = LZMA_DUMMY_MATCH;
		}
		else
		{
			range -= bound; code -= bound;
			res = LZMA_DUMMY_REP;
			prob = probabilities + ((0 + UINT32_C(0xC0)) + 12) + state;
			temp_a = *prob;

			if (range < UINT32_C(0x1000000))
			{
				if (buffer >= buffer_limit)
					return LZMA_DUMMY_ERROR;

				range <<= 8;
				code = (code << 8) | (*buffer++);
			}

			bound = (range >> 11) * temp_a;

			if (code < bound)
			{
				range = bound;
				prob = probabilities + UINT32_C(0xF0) + (state << 4) + pos_state;

				temp_a = *prob;

				if (range < UINT32_C(0x1000000))
				{
					if (buffer >= buffer_limit)
						return LZMA_DUMMY_ERROR;

					range <<= 8;
					code = (code << 8) | (*buffer++);
				}

				bound = (range >> 11) * temp_a;

				if (code < bound)
				{
					range = bound;

					if (range < UINT32_C(0x1000000))
					{
						if (buffer >= buffer_limit)
							return LZMA_DUMMY_ERROR;

						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

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
				prob = probabilities + UINT32_C(0xD8) + state;
				temp_a = *prob;

				if (range < UINT32_C(0x1000000))
				{
					if (buffer >= buffer_limit)
						return LZMA_DUMMY_ERROR;

					range <<= 8;
					code = (code << 8) | (*buffer++);
				}

				bound = (range >> 11) * temp_a;

				if (code < bound)
					range = bound;
				else
				{
					range -= bound; code -= bound;
					prob = probabilities + (UINT32_C(0xD8) + 12) + state;

					temp_a = *prob;

					if (range < UINT32_C(0x1000000))
					{
						if (buffer >= buffer_limit)
							return LZMA_DUMMY_ERROR;

						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

					bound = (range >> 11) * temp_a;

					if (code < bound)
						range = bound;
					else
					{
						range -= bound;
						code -= bound;
					}
				}
			}

			state = UINT32_C(12);
			prob = probabilities + UINT32_C(0x544);
		}

		uint32_t limit, offset;
		const uint16_t* prob_len = prob;

		temp_a = *(prob_len);

		if (range < UINT32_C(0x1000000))
		{
			if (buffer >= buffer_limit)
				return LZMA_DUMMY_ERROR;

			range <<= 8;
			code = (code << 8) | (*buffer++);
		}

		bound = (range >> 11) * temp_a;

		if (code < bound)
		{
			range = bound;
			prob_len = prob + UINT32_C(2) + (pos_state << 3);
			offset = UINT32_C(0);
			limit = UINT32_C(8);
		}
		else
		{
			range -= bound; 
			code -= bound;
			prob_len = prob + UINT32_C(1);
			temp_a = *(prob_len);

			if (range < UINT32_C(0x1000000))
			{
				if (buffer >= buffer_limit)
					return LZMA_DUMMY_ERROR;

				range <<= 8;
				code = (code << 8) | (*buffer++);
			}

			bound = (range >> 11) * temp_a;

			if (code < bound)
			{
				range = bound;
				prob_len = prob + UINT32_C(0x82) + (pos_state << 3);
				offset = limit = UINT32_C(8);
			}
			else
			{
				range -= bound; 
				code -= bound;
				prob_len = prob + UINT32_C(0x102);
				offset = UINT32_C(0x10);
				limit = UINT32_C(0x100);
			}
		}

		len = UINT32_C(1);

		do
		{
			temp_a = *(prob_len + len);

			if (range < UINT32_C(0x1000000))
			{
				if (buffer >= buffer_limit)
					return LZMA_DUMMY_ERROR;

				range <<= 8;
				code = (code << 8) | (*buffer++);
			}

			bound = (range >> 11) * temp_a;

			if (code < bound)
			{
				range = bound;
				len = (len + len);
			}
			else
			{
				range -= bound;
				code -= bound;
				len = (len + len) + UINT32_C(1);
			}
		}
		while (len < limit); len -= limit;

		len += offset;

		if (state < UINT32_C(4))
		{
			prob = probabilities + UINT32_C(0x1B0) + ((len < UINT32_C(4) ? len : UINT32_C(3)) << 6);

			uint32_t pos_slot = UINT32_C(1);

			do
			{
				temp_a = *(prob + pos_slot);

				if (range < UINT32_C(0x1000000))
				{
					if (buffer >= buffer_limit)
						return LZMA_DUMMY_ERROR;

					range <<= 8;
					code = (code << 8) | (*buffer++);
				}

				bound = (range >> 11) * temp_a;

				if (code < bound)
				{
					range = bound;
					pos_slot = (pos_slot + pos_slot);
				}
				else
				{
					range -= bound;
					code -= bound;
					pos_slot = (pos_slot + pos_slot) + UINT32_C(1);
				}
			} 
			while (pos_slot < UINT32_C(0x40));

			pos_slot -= UINT32_C(0x40);

			if (pos_slot >= UINT32_C(4))
			{
				uint32_t nr_direct_bits = (pos_slot >> 1) - UINT32_C(1);

				if (pos_slot < UINT32_C(14))
					prob = probabilities + UINT32_C(0x2B0) + ((UINT32_C(2) | (pos_slot & UINT32_C(1))) << nr_direct_bits) - pos_slot - UINT32_C(1);
				else
				{
					nr_direct_bits -= UINT32_C(4);

					do
					{
						if (range < UINT32_C(0x1000000))
						{
							if (buffer >= buffer_limit)
								return LZMA_DUMMY_ERROR;

							range <<= 8;
							code = (code << 8) | (*buffer++);
						}

						range >>= 1;
						code -= range & (((code - range) >> 31) - UINT32_C(1));

					}
					while (--nr_direct_bits);

					prob = probabilities + UINT32_C(0x322);
					nr_direct_bits = UINT32_C(4);
				}

				uint32_t i = UINT32_C(1);

				do
				{
					temp_a = *(prob + i);

					if (range < UINT32_C(0x1000000))
					{
						if (buffer >= buffer_limit)
							return LZMA_DUMMY_ERROR;

						range <<= 8;
						code = (code << 8) | (*buffer++);
					}

					bound = (range >> 11) * temp_a;

					if (code < bound)
					{
						range = bound;
						i += i;
					}
					else
					{
						range -= bound;
						code -= bound;
						i += i + UINT32_C(1);
					}
				} 
				while (--nr_direct_bits);
			}
		}
	}

	if (range < UINT32_C(0x1000000))
	{
		if (buffer >= buffer_limit)
			return LZMA_DUMMY_ERROR;

		range <<= 8;
		code = (code << 8) | (*buffer++);
	}

	return res;
}


inline static void lzma_decoder_init_dictionary_and_state(lzma_decoder_t* p, uint8_t init_dictionary, uint8_t init_state)
{
	p->need_to_flush = 1;
	p->remaining_length = p->temp_buffer_size = UINT32_C(0);

	if (init_dictionary)
	{
		p->processed_position = p->check_dictionary_size = UINT32_C(0);
		p->need_to_init_state = 1;
	}

	if (init_state)
		p->need_to_init_state = 1;
}


inline static void lzma_decoder_init(lzma_decoder_t* p)
{
	p->dictionary_position = UINT64_C(0);
	lzma_decoder_init_dictionary_and_state(p, 1, 1);
}


inline static void lzma_decoder_init_state_real(lzma_decoder_t* p)
{
	size_t i, probabilities_count = (UINT32_C(0xA36) << (p->properties.lc + p->properties.lp));
	uint16_t* probabilities = p->probabilities;

	for (i = 0; i < probabilities_count; ++i)
		probabilities[i] = UINT16_C(0x400);

	p->reps[0] = p->reps[1] = p->reps[2] = p->reps[3] = UINT32_C(1);
	p->state = UINT32_C(0);
	p->need_to_init_state = 0;
}


inline static lzma_rc_t lzma_decoder_decode_to_dictionary(lzma_decoder_t* p, size_t dictionary_limit, const uint8_t* src, size_t* src_len, lzma_finish_mode_t finish_mode, lzma_status_t* status)
{
	size_t in_size = *src_len;

	*src_len = UINT64_C(0);

	lzma_decoder_write_rem(p, dictionary_limit);

	*status = LZMA_STATUS_NOT_SPECIFIED;

	while (p->remaining_length != UINT32_C(0x112))
	{
		uint8_t check_end_mark_now;

		if (p->need_to_flush)
		{
			for (; in_size && p->temp_buffer_size < UINT32_C(5); (*src_len)++, --in_size)
				p->temp_buffer[p->temp_buffer_size++] = *src++;

			if (p->temp_buffer_size < UINT32_C(5))
			{
				*status = LZMA_STATUS_NEEDS_MORE_INPUT;

				return LZMA_RC_OK;
			}

			if (p->temp_buffer[0])
				return LZMA_RC_DATA;

			p->code = (((uint32_t)p->temp_buffer[1]) << 24) | (((uint32_t)p->temp_buffer[2]) << 16) | (((uint32_t)p->temp_buffer[3]) << 8) | (uint32_t)p->temp_buffer[4];

			p->range = UINT32_C(0xFFFFFFFF);
			p->need_to_flush = 0;
			p->temp_buffer_size = UINT32_C(0);
		}

		check_end_mark_now = 0;

		if (p->dictionary_position >= dictionary_limit)
		{
			if (!p->remaining_length && !p->code)
			{
				*status = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK;

				return LZMA_RC_OK;
			}

			if (finish_mode == LZMA_FINISH_ANY)
			{
				*status = LZMA_STATUS_NOT_FINISHED;

				return LZMA_RC_OK;
			}

			if (p->remaining_length)
			{
				*status = LZMA_STATUS_NOT_FINISHED;

				return LZMA_RC_DATA;
			}

			check_end_mark_now = 1;
		}

		if (p->need_to_init_state)
			lzma_decoder_init_state_real(p);

		if (!p->temp_buffer_size)
		{
			size_t processed;
			const uint8_t* buffer_limit;

			if (in_size < UINT64_C(20) || check_end_mark_now)
			{
				lzma_dummy_t dummy_rc = lzma_decoder_try_dummy(p, src, in_size);

				if (dummy_rc == LZMA_DUMMY_ERROR)
				{
					lzma_memcpy(p->temp_buffer, src, in_size);

					p->temp_buffer_size = (uint32_t)in_size;
					*src_len += in_size;
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
			else buffer_limit = src + in_size - UINT64_C(20);

			p->buffer = src;

			if (lzma_decoder_decode_real_ex(p, dictionary_limit, buffer_limit))
				return LZMA_RC_DATA;

			processed = (size_t)(p->buffer - src);
			*src_len += processed;
			src += processed;
			in_size -= processed;
		}
		else
		{
			uint32_t rem = p->temp_buffer_size, look_ahead = UINT32_C(0);

			while (rem < UINT32_C(20) && look_ahead < in_size)
				p->temp_buffer[rem++] = src[look_ahead++];

			p->temp_buffer_size = rem;

			if (rem < UINT32_C(20) || check_end_mark_now)
			{
				lzma_dummy_t dummy_rc = lzma_decoder_try_dummy(p, p->temp_buffer, rem);

				if (dummy_rc == LZMA_DUMMY_ERROR)
				{
					*src_len += look_ahead;
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

			if (lzma_decoder_decode_real_ex(p, dictionary_limit, p->buffer))
				return LZMA_RC_DATA;

			uint32_t temp_b = (uint32_t)(p->buffer - p->temp_buffer);

			if (rem < temp_b)
				return LZMA_RC_FAILURE;

			rem -= temp_b;

			if (look_ahead < rem)
				return LZMA_RC_FAILURE;

			look_ahead -= rem;

			*src_len += look_ahead;
			src += look_ahead;
			in_size -= look_ahead;
			p->temp_buffer_size = UINT32_C(0);
		}
	}

	if (!p->code)
		*status = LZMA_STATUS_FINISHED_WITH_MARK;

	return (!p->code) ? LZMA_RC_OK : LZMA_RC_DATA;
}


inline static lzma_rc_t lzma_decoder_decode_to_buffer(lzma_decoder_t* p, uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t* src_len, lzma_finish_mode_t finish_mode, lzma_status_t* status)
{
	size_t out_size = *dest_len;
	size_t in_size = *src_len;

	*src_len = *dest_len = UINT64_C(0);

	for (;;)
	{
		size_t in_size_cur = in_size, out_size_cur, dictionary_position;
		lzma_finish_mode_t cur_finish_mode;
		lzma_rc_t rc;

		if (p->dictionary_position == p->dictionary_buffer_size)
			p->dictionary_position = UINT64_C(0);

		dictionary_position = p->dictionary_position;

		if (out_size > p->dictionary_buffer_size - dictionary_position)
		{
			out_size_cur = p->dictionary_buffer_size;
			cur_finish_mode = LZMA_FINISH_ANY;
		}
		else
		{
			out_size_cur = dictionary_position + out_size;
			cur_finish_mode = finish_mode;
		}

		rc = lzma_decoder_decode_to_dictionary(p, out_size_cur, src, &in_size_cur, cur_finish_mode, status);

		src += in_size_cur;
		in_size -= in_size_cur;
		*src_len += in_size_cur;
		out_size_cur = p->dictionary_position - dictionary_position;

		lzma_memcpy(dest, p->dictionary + dictionary_position, out_size_cur);

		dest += out_size_cur;
		out_size -= out_size_cur;
		*dest_len += out_size_cur;

		if (rc) return rc;

		if (!out_size_cur || !out_size)
			return LZMA_RC_OK;
	}
}


inline static void lzma_decoder_release_probabilities(lzma_decoder_t* p, lzma_allocator_ptr_t allocator)
{
	allocator->release(allocator, p->probabilities);
	p->probabilities = NULL;
}


inline static void lzma_decoder_free_dictionary(lzma_decoder_t* p, lzma_allocator_ptr_t allocator)
{
	allocator->release(allocator, p->dictionary);
	p->dictionary = NULL;
}


inline static void lzma_decoder_release(lzma_decoder_t* p, lzma_allocator_ptr_t allocator)
{
	lzma_decoder_release_probabilities(p, allocator);
	lzma_decoder_free_dictionary(p, allocator);
}


inline static lzma_rc_t lzma_properties_decode(lzma_properties_t* p, const uint8_t* data, uint32_t size)
{
	uint32_t dictionary_size;
	uint8_t d;

	if (size < UINT32_C(5)) 
		return LZMA_RC_UNSUPPORTED;
	else dictionary_size = ((uint32_t)data[1]) | (((uint32_t)data[2]) << 8) | (((uint32_t)data[3]) << 16) | (((uint32_t)data[4]) << 24);

	if (dictionary_size < UINT32_C(0x1000))
		dictionary_size = UINT32_C(0x1000);

	p->dictionary_size = dictionary_size;

	d = data[0];

	if (d >= UINT8_C(225))
		return LZMA_RC_UNSUPPORTED;

	p->lc = d % UINT8_C(9);
	d /= UINT8_C(9);
	p->pb = d / UINT8_C(5);
	p->lp = d % UINT8_C(5);

	return LZMA_RC_OK;
}


inline static lzma_rc_t lzma_decoder_allocate_probabilities_ex(lzma_decoder_t* p, const lzma_properties_t* new_prop, lzma_allocator_ptr_t allocator)
{
	uint32_t probabilities_count = UINT32_C(0xA36) << (new_prop->lc + new_prop->lp);

	if (!p->probabilities || probabilities_count != p->probabilities_count)
	{
		lzma_decoder_release_probabilities(p, allocator);

		p->probabilities = (uint16_t*)allocator->allocate(allocator->state, probabilities_count * sizeof(uint16_t));
		p->probabilities_count = probabilities_count;

		if (!p->probabilities)
			return LZMA_RC_MEMORY;
	}

	return LZMA_RC_OK;
}


inline static lzma_rc_t lzma_decoder_allocate_probabilities(lzma_decoder_t* p, const uint8_t *properties, uint32_t properties_size, lzma_allocator_ptr_t allocator)
{
	lzma_rc_t rc;
	lzma_properties_t new_prop;

	rc = lzma_properties_decode(&new_prop, properties, properties_size); 

	if (rc) return rc;

	rc = lzma_decoder_allocate_probabilities_ex(p, &new_prop, allocator); 

	if (rc) return rc;

	p->properties = new_prop;

	return LZMA_RC_OK;
}


inline static lzma_rc_t lzma_decoder_allocate(lzma_decoder_t* p, const uint8_t *properties, uint32_t properties_size, lzma_allocator_ptr_t allocator)
{
	lzma_rc_t rc;
	lzma_properties_t new_prop;
	size_t dictionary_buffer_size;

	rc = lzma_properties_decode(&new_prop, properties, properties_size);

	if (rc) return rc;

	rc = lzma_decoder_allocate_probabilities_ex(p, &new_prop, allocator);

	if (rc) return rc;

	uint32_t dictionary_size = new_prop.dictionary_size;
	size_t mask = UINT64_C(0x3FFFFFFF);

	if (dictionary_size >= UINT32_C(0x40000000))
		mask = UINT32_C(0x3FFFFF);
	else if (dictionary_size >= UINT32_C(0x400000))
		mask = UINT32_C(0xFFFFF);

	dictionary_buffer_size = ((size_t)dictionary_size + mask) & ~mask;

	if (dictionary_buffer_size < dictionary_size)
		dictionary_buffer_size = dictionary_size;

	if (!p->dictionary || dictionary_buffer_size != p->dictionary_buffer_size)
	{
		lzma_decoder_free_dictionary(p, allocator);

		p->dictionary = (uint8_t*)allocator->allocate(allocator->state, dictionary_buffer_size);

		if (!p->dictionary)
		{
			lzma_decoder_release_probabilities(p, allocator);

			return LZMA_RC_MEMORY;
		}
	}

	p->dictionary_buffer_size = dictionary_buffer_size;
	p->properties = new_prop;

	return LZMA_RC_OK;
}


inline static void lzma_decoder_update_with_uncompressed(lzma_decoder_t* p, const uint8_t* src, size_t size)
{
	lzma_memcpy(p->dictionary + p->dictionary_position, src, size);

	p->dictionary_position += size;

	if (!p->check_dictionary_size && p->properties.dictionary_size - p->processed_position <= size)
		p->check_dictionary_size = p->properties.dictionary_size;

	p->processed_position += (uint32_t)size;
}


inline static void lzma_decoder_init_dictionary_and_state(lzma_decoder_t* p, uint8_t init_dictionary, uint8_t init_state);


typedef uint32_t lzma_lz_ref_t;


typedef struct lzma_match_finder_s
{
	uint8_t* buffer;
	uint32_t position;
	uint32_t position_limit;
	uint32_t stream_position;
	uint32_t length_limit;
	uint32_t cyclic_buffer_position;
	uint32_t cyclic_buffer_size;
	uint8_t stream_end_was_reached;
	uint8_t mode_bt;
	uint8_t big_hash;
	uint8_t direct_input;
	uint32_t match_max_length;
	lzma_lz_ref_t* hash;
	lzma_lz_ref_t* child;
	uint32_t hash_mask;
	uint32_t cut_value;
	uint8_t* buffer_base;
	lzma_seq_in_strm_buf_t* stream;
	uint32_t block_size;
	uint32_t keep_size_before;
	uint32_t keep_size_after;
	uint32_t hash_byte_count;
	size_t direct_input_remaining;
	uint32_t history_size;
	uint32_t fixed_hash_size;
	uint32_t hash_size_sum;
	lzma_rc_t result;
	uint32_t crc[256];
	size_t references_count;
	uint64_t expected_data_size;
}
lzma_match_finder_t;


inline static uint8_t lzma_mf_need_move(lzma_match_finder_t* p);
inline static uint8_t* lzma_mf_get_ptr_to_current_position(lzma_match_finder_t* p);
inline static void lzma_mf_move_block(lzma_match_finder_t* p);
inline static void lzma_mf_read_if_required(lzma_match_finder_t* p);
inline static void lzma_mf_construct(lzma_match_finder_t* p);
inline static uint8_t lzma_mf_create(lzma_match_finder_t* p, uint32_t history_size, uint32_t keep_add_buffer_before, uint32_t match_max_length, uint32_t keep_add_buffer_after, lzma_allocator_ptr_t allocator);
inline static void lzma_mf_free(lzma_match_finder_t* p, lzma_allocator_ptr_t allocator);
inline static void lzma_mf_normalize_3(uint32_t sub_value, lzma_lz_ref_t* items, size_t items_count);
inline static void lzma_mf_reduce_offsets(lzma_match_finder_t* p, uint32_t sub_value);
inline static uint32_t* lzma_mf_get_matches_spec_a(uint32_t length_limit, uint32_t current_match, uint32_t pos, const uint8_t* buffer, lzma_lz_ref_t* child, uint32_t cyclic_buffer_position, uint32_t cyclic_buffer_size, uint32_t cut_value, uint32_t* distances, uint32_t max_length);
inline static void lzma_mf_init_low_hash(lzma_match_finder_t* p);
inline static void lzma_mf_init_high_hash(lzma_match_finder_t* p);
inline static void lzma_mf_init_3(lzma_match_finder_t* p, uint8_t read_data);
inline static void lzma_mf_init(lzma_match_finder_t* p);
inline static uint32_t lzma_mf_bt_3_zip_get_matches(lzma_match_finder_t* p, uint32_t* distances);
inline static uint32_t lzma_mf_hc_3_zip_get_matches(lzma_match_finder_t* p, uint32_t* distances);
inline static void lzma_mf_bt_3_zip_skip(lzma_match_finder_t* p, uint32_t num);
inline static void lzma_mf_hc_3_zip_skip(lzma_match_finder_t* p, uint32_t num);


inline static void lzma_lz_in_window_free(lzma_match_finder_t* p, lzma_allocator_ptr_t allocator)
{
	if (!p->direct_input)
	{
		allocator->release(allocator, p->buffer_base);
		p->buffer_base = NULL;
	}
}


inline static uint8_t lzma_lz_in_window_create(lzma_match_finder_t* p, uint32_t keep_size_reserve, lzma_allocator_ptr_t allocator)
{
	uint32_t block_size = p->keep_size_before + p->keep_size_after + keep_size_reserve;

	if (p->direct_input)
	{
		p->block_size = block_size;

		return 1;
	}

	if (!p->buffer_base || p->block_size != block_size)
	{
		lzma_lz_in_window_free(p, allocator);

		p->block_size = block_size;
		p->buffer_base = (uint8_t*)allocator->allocate(allocator->state, (size_t)block_size);
	}

	return (p->buffer_base != NULL);
}


inline static uint8_t* lzma_mf_get_ptr_to_current_position(lzma_match_finder_t* p) { return p->buffer; }
inline static uint32_t lzma_mf_get_num_available_bytes(lzma_match_finder_t* p) { return p->stream_position - p->position; }


inline static void lzma_mf_reduce_offsets(lzma_match_finder_t* p, uint32_t sub_value)
{
	p->position_limit -= sub_value;
	p->position -= sub_value;
	p->stream_position -= sub_value;
}


inline static lzma_rc_t lzma_seq_in_strm_buf_read(lzma_seq_in_strm_buf_t* p, uint8_t* dest, size_t* size)
{
	if (p->position >= p->buffer_size)
	{
		*size = UINT64_C(0);
		return LZMA_RC_INPUT_EOF;
	}

	if (*size == UINT64_C(0))
		return LZMA_RC_OK;

	uint8_t* offset = p->buffer + p->position;
	size_t available = p->buffer_size - p->position;
	size_t to_read = available;

	if (available >= *size)
		to_read = *size;

	lzma_memcpy(dest, offset, to_read);

	p->position += to_read;
	*size = to_read;

	return LZMA_RC_OK;
}


inline static void lzma_mf_read_block(lzma_match_finder_t* p)
{
	if (p->stream_end_was_reached || p->result != LZMA_RC_OK)
		return;

	if (p->direct_input)
	{
		uint32_t cur_size = UINT32_C(0xFFFFFFFF) - (p->stream_position - p->position);

		if (cur_size > p->direct_input_remaining)
			cur_size = (uint32_t)p->direct_input_remaining;

		p->direct_input_remaining -= cur_size;
		p->stream_position += cur_size;

		if (!p->direct_input_remaining)
			p->stream_end_was_reached = 1;

		return;
	}

	for (;;)
	{
		uint8_t* dest = p->buffer + (p->stream_position - p->position);
		size_t size = (size_t)(p->buffer_base + p->block_size - dest);

		if (!size) return;
		
		p->result = lzma_seq_in_strm_buf_read(p->stream, dest, &size);

		if (p->result != LZMA_RC_OK)
			return;

		if (!size)
		{
			p->stream_end_was_reached = 1;

			return;
		}

		p->stream_position += (uint32_t)size;

		if (p->stream_position - p->position > p->keep_size_after)
			return;
	}
}


inline static void lzma_mf_move_block(lzma_match_finder_t* p)
{
	lzma_memmove(p->buffer_base, p->buffer - p->keep_size_before, (size_t)(p->stream_position - p->position) + p->keep_size_before);

	p->buffer = p->buffer_base + p->keep_size_before;
}


inline static uint8_t lzma_mf_need_move(lzma_match_finder_t* p)
{
	if (p->direct_input) return 0;

	return ((p->buffer_base + p->block_size - p->buffer) <= p->keep_size_after);
}


inline static void lzma_mf_read_if_required(lzma_match_finder_t* p)
{
	if (p->stream_end_was_reached)
		return;

	if (p->keep_size_after >= p->stream_position - p->position)
		lzma_mf_read_block(p);
}


inline static void lzma_mf_check_move_and_read(lzma_match_finder_t* p)
{
	if (lzma_mf_need_move(p))
		lzma_mf_move_block(p);

	lzma_mf_read_block(p);
}


inline static void lzma_mf_set_defaults(lzma_match_finder_t* p)
{
	p->cut_value = UINT32_C(32);
	p->mode_bt = 1;
	p->hash_byte_count = UINT32_C(4);
	p->big_hash = 0;
}


inline static void lzma_mf_construct(lzma_match_finder_t* p)
{
	uint32_t i;

	p->buffer_base = NULL;
	p->direct_input = 1;
	p->hash = NULL;
	p->expected_data_size = (uint64_t)INT64_C(-1);
	lzma_mf_set_defaults(p);

	for (i = 0; i < UINT32_C(0x100); ++i)
	{
		uint32_t j, r = i;

		for (j = UINT32_C(0); j < UINT32_C(8); ++j)
			r = (r >> 1) ^ (UINT32_C(0xEDB88320) & (UINT32_C(0) - (r & UINT32_C(1))));

		p->crc[i] = r;
	}
}


inline static void lzma_mf_free_object_memory(lzma_match_finder_t* p, lzma_allocator_ptr_t allocator)
{
	allocator->release(allocator, p->hash);
	p->hash = NULL;
}


inline static void lzma_mf_free(lzma_match_finder_t* p, lzma_allocator_ptr_t allocator)
{
	lzma_mf_free_object_memory(p, allocator);
	lzma_lz_in_window_free(p, allocator);
}


inline static lzma_lz_ref_t* lzma_mf_allocate_refs(size_t num, lzma_allocator_ptr_t allocator)
{
	size_t size_in_bytes = num * sizeof(lzma_lz_ref_t);

	if (size_in_bytes / sizeof(lzma_lz_ref_t) != num)
		return NULL;

	return (lzma_lz_ref_t*)allocator->allocate(allocator->state, size_in_bytes);
}


uint8_t lzma_mf_create(lzma_match_finder_t* p, uint32_t history_size, uint32_t keep_add_buffer_before, uint32_t match_max_length, uint32_t keep_add_buffer_after, lzma_allocator_ptr_t allocator)
{
	uint32_t size_reserve;

	if (history_size > UINT32_C(0xE0000000))
	{
		lzma_mf_free(p, allocator);

		return 0;
	}

	size_reserve = history_size >> 1;

	if (history_size >= UINT32_C(0xC0000000))
		size_reserve = history_size >> 3;
	else if (history_size >= UINT32_C(0x80000000))
		size_reserve = history_size >> 2;

	size_reserve += (keep_add_buffer_before + match_max_length + keep_add_buffer_after) / UINT32_C(0x80002);
	p->keep_size_before = history_size + keep_add_buffer_before + UINT32_C(1);
	p->keep_size_after = match_max_length + keep_add_buffer_after;

	if (lzma_lz_in_window_create(p, size_reserve, allocator))
	{
		uint32_t hs, new_cyclic_buffer_size = history_size + UINT32_C(1);

		p->match_max_length = match_max_length;


		p->fixed_hash_size = UINT32_C(0);

		if (p->hash_byte_count == UINT32_C(2))
			hs = UINT32_C(0xFFFF);
		else
		{
			hs = history_size;

			if (hs > p->expected_data_size)
				hs = (uint32_t)p->expected_data_size;

			if (hs) hs--;

			hs |= (hs >> 1);
			hs |= (hs >> 2);
			hs |= (hs >> 4);
			hs |= (hs >> 8);

			hs >>= 1;

			hs |= UINT32_C(0xFFFF);

			if (hs > UINT32_C(0x1000000))
			{
				if (p->hash_byte_count == UINT32_C(3))
					hs = UINT32_C(0xFFFFFF);
				else hs >>= 1;
			}
		}

		p->hash_mask = hs;
		hs++;

		if (p->hash_byte_count > UINT32_C(2)) p->fixed_hash_size += UINT32_C(0x400);
		if (p->hash_byte_count > UINT32_C(3)) p->fixed_hash_size += UINT32_C(0x10000);
		if (p->hash_byte_count > UINT32_C(4)) p->fixed_hash_size += UINT32_C(0x100000);

		hs += p->fixed_hash_size;

		size_t new_size, child_count;

		p->history_size = history_size;
		p->hash_size_sum = hs;
		p->cyclic_buffer_size = new_cyclic_buffer_size;

		child_count = new_cyclic_buffer_size;

		if (p->mode_bt)
			child_count <<= 1;

		new_size = hs + child_count;

		if (p->hash && p->references_count == new_size)
			return 1;

		lzma_mf_free_object_memory(p, allocator);

		p->references_count = new_size;

		p->hash = lzma_mf_allocate_refs(new_size, allocator);

		if (p->hash)
		{
			p->child = p->hash + p->hash_size_sum;

			return 1;
		}
	}

	lzma_mf_free(p, allocator);

	return 0;
}


inline static void lzma_mf_set_limits(lzma_match_finder_t* p)
{
	uint32_t limit = UINT32_C(0xFFFFFFFF) - p->position;
	uint32_t limit_b = p->cyclic_buffer_size - p->cyclic_buffer_position;

	if (limit_b < limit)
		limit = limit_b;

	limit_b = p->stream_position - p->position;

	if (limit_b <= p->keep_size_after)
	{
		if (limit_b)
			limit_b = UINT32_C(1);
	}
	else limit_b -= p->keep_size_after;

	if (limit_b < limit)
		limit = limit_b;

	uint32_t length_limit = p->stream_position - p->position;

	if (length_limit > p->match_max_length)
		length_limit = p->match_max_length;

	p->length_limit = length_limit;
	p->position_limit = p->position + limit;
}


inline static void lzma_mf_init_low_hash(lzma_match_finder_t* p)
{
	lzma_lz_ref_t* items = p->hash;
	size_t i, items_count = p->fixed_hash_size;

	for (i = UINT64_C(0); i < items_count; ++i)
		items[i] = UINT32_C(0);
}


inline static void lzma_mf_init_high_hash(lzma_match_finder_t* p)
{
	lzma_lz_ref_t* items = p->hash + p->fixed_hash_size;
	size_t i, items_count = (size_t)p->hash_mask + UINT32_C(1);

	for (i = UINT64_C(0); i < items_count; ++i)
		items[i] = UINT32_C(0);
}


inline static void lzma_mf_init_3(lzma_match_finder_t* p, uint8_t read_data)
{
	p->cyclic_buffer_position = UINT32_C(0);
	p->buffer = p->buffer_base;
	p->position = p->stream_position = p->cyclic_buffer_size;
	p->result = LZMA_RC_OK;
	p->stream_end_was_reached = 0;

	if (read_data)
		lzma_mf_read_block(p);

	lzma_mf_set_limits(p);
}


inline static void lzma_mf_init(lzma_match_finder_t* p)
{
	lzma_mf_init_high_hash(p);
	lzma_mf_init_low_hash(p);
	lzma_mf_init_3(p, 1);
}


inline static uint32_t lzma_mf_get_sub_value(lzma_match_finder_t* p)
{
	return (p->position - p->history_size - UINT32_C(1)) & ~UINT32_C(0x3FF);
}


inline static void lzma_mf_normalize_3(uint32_t sub_value, lzma_lz_ref_t* items, size_t items_count)
{
	size_t i;

	for (i = UINT64_C(0); i < items_count; ++i)
	{
		uint32_t value = items[i];

		if (value <= sub_value)
			value = UINT32_C(0);
		else value -= sub_value;

		items[i] = value;
	}
}


inline static void lzma_mf_normalize(lzma_match_finder_t* p)
{
	uint32_t sub_value = lzma_mf_get_sub_value(p);

	lzma_mf_normalize_3(sub_value, p->hash, p->references_count);
	lzma_mf_reduce_offsets(p, sub_value);
}


inline static void lzma_mf_check_limits(lzma_match_finder_t* p)
{
	if (p->position == (UINT32_C(0xFFFFFFFF)))
		lzma_mf_normalize(p);

	if (!p->stream_end_was_reached && p->keep_size_after == p->stream_position - p->position)
		lzma_mf_check_move_and_read(p);

	if (p->cyclic_buffer_position == p->cyclic_buffer_size)
		p->cyclic_buffer_position = UINT32_C(0);

	lzma_mf_set_limits(p);
}


inline static uint32_t* lzma_mf_hc_get_matches_spec(uint32_t length_limit, uint32_t current_match, uint32_t pos, const uint8_t* cur, lzma_lz_ref_t* child, uint32_t cyclic_buffer_position, uint32_t cyclic_buffer_size, uint32_t cut_value, uint32_t* distances, uint32_t max_length)
{
	child[cyclic_buffer_position] = current_match;

	for (;;)
	{
		uint32_t delta = pos - current_match;

		if (!(cut_value--) || delta >= cyclic_buffer_size)
			return distances;

		const uint8_t* pb = cur - delta;
		current_match = child[cyclic_buffer_position - delta + ((delta > cyclic_buffer_position) ? cyclic_buffer_size : UINT32_C(0))];

		if (pb[max_length] == cur[max_length] && *pb == *cur)
		{
			uint32_t len = UINT32_C(0);

			while (++len != length_limit)
				if (pb[len] != cur[len])
					break;

			if (max_length < len)
			{
				*distances++ = max_length = len;
				*distances++ = delta - UINT32_C(1);

				if (len == length_limit)
					return distances;
			}
		}
	}
}


inline static uint32_t* lzma_mf_get_matches_spec_a(uint32_t length_limit, uint32_t current_match, uint32_t pos, const uint8_t* cur, lzma_lz_ref_t* child, uint32_t cyclic_buffer_position, uint32_t cyclic_buffer_size, uint32_t cut_value, uint32_t* distances, uint32_t max_length)
{
	lzma_lz_ref_t* ptr_a = child + (cyclic_buffer_position << 1) + UINT32_C(1);
	lzma_lz_ref_t* ptr_b = child + (cyclic_buffer_position << 1);
	uint32_t len_a = UINT32_C(0), len_b = UINT32_C(0);

	for (;;)
	{
		uint32_t delta = pos - current_match;

		if (cut_value-- == UINT32_C(0) || delta >= cyclic_buffer_size)
		{
			*ptr_a = *ptr_b = UINT32_C(0);

			return distances;
		}

		lzma_lz_ref_t* pair = child + ((cyclic_buffer_position - delta + ((delta > cyclic_buffer_position) ? cyclic_buffer_size : UINT32_C(0))) << 1);
		const uint8_t* pb = cur - delta;
		uint32_t len = (len_a < len_b) ? len_a : len_b;

		if (pb[len] == cur[len])
		{
			if (++len != length_limit && pb[len] == cur[len])
				while (++len != length_limit)
					if (pb[len] != cur[len])
						break;

			if (max_length < len)
			{
				*distances++ = max_length = len;
				*distances++ = delta - UINT32_C(1);

				if (len == length_limit)
				{
					*ptr_b = pair[0];
					*ptr_a = pair[1];

					return distances;
				}
			}
		}

		if (pb[len] < cur[len])
		{
			*ptr_b = current_match;
			ptr_b = pair + UINT32_C(1);
			current_match = *ptr_b;
			len_b = len;
		}
		else
		{
			*ptr_a = current_match;
			ptr_a = pair;
			current_match = *ptr_a;
			len_a = len;
		}
	}
}


inline static void lzma_mf_skip_matches_spec(uint32_t length_limit, uint32_t current_match, uint32_t pos, const uint8_t* cur, lzma_lz_ref_t* child, uint32_t cyclic_buffer_position, uint32_t cyclic_buffer_size, uint32_t cut_value)
{
	lzma_lz_ref_t *ptr_a = child + (cyclic_buffer_position << 1) + UINT32_C(1);
	lzma_lz_ref_t *ptr_b = child + (cyclic_buffer_position << 1);
	uint32_t len_a = UINT32_C(0), len_b = UINT32_C(0);

	for (;;)
	{
		uint32_t delta = pos - current_match;

		if (cut_value-- == UINT32_C(0) || delta >= cyclic_buffer_size)
		{
			*ptr_a = *ptr_b = UINT32_C(0);
			return;
		}

		lzma_lz_ref_t* pair = child + ((cyclic_buffer_position - delta + ((delta > cyclic_buffer_position) ? cyclic_buffer_size : UINT32_C(0))) << 1);
		const uint8_t* pb = cur - delta;
		uint32_t len = (len_a < len_b ? len_a : len_b);

		if (pb[len] == cur[len])
		{
			while (++len != length_limit)
				if (pb[len] != cur[len])
					break;

			if (len == length_limit)
			{
				*ptr_b = pair[0];
				*ptr_a = pair[1];

				return;
			}
		}

		if (pb[len] < cur[len])
		{
			*ptr_b = current_match;
			ptr_b = pair + UINT32_C(1);
			current_match = *ptr_b;
			len_b = len;
		}
		else
		{
			*ptr_a = current_match;
			ptr_a = pair;
			current_match = *ptr_a;
			len_a = len;
		}
	}
}


inline static void lzma_mf_move_position(lzma_match_finder_t* p) 
{ 
	++p->cyclic_buffer_position; 
	p->buffer++; 

	if (++p->position == p->position_limit) 
		lzma_mf_check_limits(p); 
}


inline static uint32_t lzma_mf_bt_2_get_matches(lzma_match_finder_t* p, uint32_t* distances)
{
	uint32_t offset;

	uint32_t hv, length_limit, current_match;
	const uint8_t* cur; 

	length_limit = p->length_limit; 
	
	if (length_limit < 2) 
	{ 
		lzma_mf_move_position(p); 
		
		return UINT32_C(0);
	}
	
	cur = p->buffer;

	hv = cur[0] | (((uint32_t)cur[1]) << 8);

	current_match = p->hash[hv];
	p->hash[hv] = p->position;
	offset = UINT32_C(0);

	offset = (uint32_t)(lzma_mf_get_matches_spec_a(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value, distances + offset, UINT32_C(1)) - distances); 
	
	++p->cyclic_buffer_position;
	p->buffer++; 
	
	if (++p->position == p->position_limit) 
		lzma_mf_check_limits(p); 
	
	return offset;
}


inline static uint32_t lzma_mf_bt_3_zip_get_matches(lzma_match_finder_t* p, uint32_t* distances)
{
	uint32_t hv, offset, length_limit, current_match;
	const uint8_t* cur; 

	length_limit = p->length_limit; 
	
	if (length_limit < UINT32_C(3))
	{ 
		lzma_mf_move_position(p); 
		
		return UINT32_C(0);
	}
	
	cur = p->buffer;

	hv = ((cur[2] | (((uint32_t)cur[0]) << 8)) ^ p->crc[cur[1]]) & UINT32_C(0xFFFF);

	current_match = p->hash[hv];
	p->hash[hv] = p->position;
	offset = UINT32_C(0);

	offset = (uint32_t)(lzma_mf_get_matches_spec_a(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value, distances + offset, UINT32_C(2)) - distances);
	
	++p->cyclic_buffer_position; 
	p->buffer++; 
	
	if (++p->position == p->position_limit) 
		lzma_mf_check_limits(p); 
	
	return offset;
}


inline static uint32_t lzma_mf_bt_3_get_matches(lzma_match_finder_t* p, uint32_t* distances)
{
	uint32_t hv, hc, dc, max_length, offset, pos, length_limit, current_match;
	uint32_t* hash;
	const uint8_t* cur; 

	length_limit = p->length_limit; 
	
	if (length_limit < UINT32_C(3))
	{ 
		lzma_mf_move_position(p); 

		return UINT32_C(0);
	}
	
	cur = p->buffer;

	uint32_t temp = p->crc[cur[0]] ^ cur[1]; 
	
	hc = temp & UINT32_C(0x3FF); 
	hv = (temp ^ (((uint32_t)cur[2]) << 8)) & p->hash_mask;

	hash = p->hash;
	pos = p->position;

	dc = pos - hash[hc];

	current_match = (hash + UINT32_C(0x400))[hv];

	hash[hc] = pos;

	(hash + UINT32_C(0x400))[hv] = pos;

	max_length = UINT32_C(2);
	offset = UINT32_C(0);

	if (dc < p->cyclic_buffer_size && *(cur - dc) == *cur)
	{
		ptrdiff_t diff = (ptrdiff_t)(UINT32_C(0) - dc);
		const uint8_t* c = cur + max_length; 
		const uint8_t* lim = cur + length_limit; 
		
		for (; c != lim; ++c) 
			if (*(c + diff) != *c) 
				break; 
		
		max_length = (uint32_t)(c - cur);

		distances[0] = max_length;
		distances[1] = dc - UINT32_C(1);
		offset = UINT32_C(2);

		if (max_length == length_limit)
		{
			lzma_mf_skip_matches_spec(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value);
			
			++p->cyclic_buffer_position; 
			p->buffer++; 
			
			if (++p->position == p->position_limit) 
				lzma_mf_check_limits(p); 
			
			return offset;
		}
	}

	offset = (uint32_t)(lzma_mf_get_matches_spec_a(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value, distances + offset, max_length) - distances); 
	
	++p->cyclic_buffer_position; 
	p->buffer++; 
	
	if (++p->position == p->position_limit) 
		lzma_mf_check_limits(p); 
	
	return offset;
}


inline static uint32_t lzma_mf_bt_4_get_matches(lzma_match_finder_t* p, uint32_t* distances)
{
	uint32_t hv, hc, hd, dc, dd, max_length, offset, pos, length_limit, current_match;
	uint32_t* hash;
	const uint8_t* cur; 

	length_limit = p->length_limit;
	
	if (length_limit < UINT32_C(4))
	{ 
		lzma_mf_move_position(p); 
		
		return UINT32_C(0); 
	}
	
	cur = p->buffer;

	uint32_t temp = p->crc[cur[0]] ^ cur[1]; 

	hc = temp & UINT32_C(0x3FF); 
	temp ^= ((uint32_t)cur[2] << 8); 
	hd = temp & UINT32_C(0xFFFF); 
	hv = (temp ^ (p->crc[cur[3]] << 5)) & p->hash_mask;

	hash = p->hash;
	pos = p->position;

	dc = pos - hash[hc];
	dd = pos - (hash + UINT32_C(0x400))[hd];

	current_match = (hash + UINT32_C(0x10400))[hv];

	hash[hc] = pos;
	(hash + UINT32_C(0x400))[hd] = pos;
	(hash + UINT32_C(0x10400))[hv] = pos;

	max_length = offset = UINT32_C(0);

	if (dc < p->cyclic_buffer_size && *(cur - dc) == *cur)
	{
		distances[0] = max_length = UINT32_C(2);
		distances[1] = dc - UINT32_C(1);
		offset = UINT32_C(2);
	}

	if (dc != dd && dd < p->cyclic_buffer_size && *(cur - dd) == *cur)
	{
		max_length = UINT32_C(3);
		distances[(size_t)(offset + UINT32_C(1))] = dd - UINT32_C(1);
		offset += UINT32_C(2);
		dc = dd;
	}

	if (offset)
	{
		ptrdiff_t diff = (ptrdiff_t)(INT64_C(0) - (int64_t)dc); 
		const uint8_t* c = cur + max_length; 
		const uint8_t* lim = cur + length_limit; 
		
		for (; c != lim; ++c) 
			if (*(c + diff) != *c) 
				break; 
		
		max_length = (uint32_t)(c - cur);

		distances[offset - UINT32_C(2)] = max_length;

		if (max_length == length_limit)
		{
			lzma_mf_skip_matches_spec(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value);
			
			++p->cyclic_buffer_position; 
			p->buffer++; 
			
			if (++p->position == p->position_limit) 
				lzma_mf_check_limits(p); 
			
			return offset;
		}
	}

	if (max_length < UINT32_C(3))
		max_length = UINT32_C(3);

	offset = (uint32_t)(lzma_mf_get_matches_spec_a(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value, distances + offset, max_length) - distances); 
	
	++p->cyclic_buffer_position; 
	p->buffer++; 
	
	if (++p->position == p->position_limit) 
		lzma_mf_check_limits(p); 
	
	return offset;
}


inline static uint32_t lzma_mf_hc_4_get_matches(lzma_match_finder_t* p, uint32_t* distances)
{
	uint32_t hc, hd, dc, dd, max_length, offset, pos;
	uint32_t* hash;
	uint32_t hv, length_limit; 
	const uint8_t* cur; 
	uint32_t current_match; 
	
	length_limit = p->length_limit;
	
	if (length_limit < UINT32_C(4))
	{ 
		lzma_mf_move_position(p); 
		
		return UINT32_C(0);
	}
	
	cur = p->buffer;

	uint32_t temp = p->crc[cur[0]] ^ cur[1]; 

	hc = temp & UINT32_C(0x3FF); 
	temp ^= (((uint32_t)cur[2]) << 8); 
	hd = temp & UINT32_C(0xFFFF); 
	hv = (temp ^ (p->crc[cur[3]] << 5)) & p->hash_mask;

	hash = p->hash;
	pos = p->position;

	dc = pos - hash[hc];
	dd = pos - (hash + UINT32_C(0x400))[hd];

	current_match = (hash + UINT32_C(0x10400))[hv];

	hash[hc] = pos;
	(hash + UINT32_C(0x400))[hd] = pos;
	(hash + UINT32_C(0x10400))[hv] = pos;

	max_length = offset = UINT32_C(0);

	if (dc < p->cyclic_buffer_size && *(cur - dc) == *cur)
	{
		distances[0] = max_length = UINT32_C(2);
		distances[1] = dc - UINT32_C(1);
		offset = UINT32_C(2);
	}

	if (dc != dd && dd < p->cyclic_buffer_size && *(cur - dd) == *cur)
	{
		max_length = UINT32_C(3);
		distances[offset + UINT32_C(1)] = dd - UINT32_C(1);
		offset += UINT32_C(2);
		dc = dd;
	}

	if (offset)
	{
		ptrdiff_t diff = (ptrdiff_t)(INT64_C(0) - (int64_t)dc); 
		const uint8_t* c = cur + max_length; 
		const uint8_t* lim = cur + length_limit; 
		
		for (; c != lim; ++c) 
			if (*(c + diff) != *c) 
				break; 
		
		max_length = (uint32_t)(c - cur);
		distances[offset - UINT32_C(2)] = max_length;

		if (max_length == length_limit)
		{
			p->child[p->cyclic_buffer_position] = current_match;
			++p->cyclic_buffer_position; 
			p->buffer++; 
			
			if (++p->position == p->position_limit) 
				lzma_mf_check_limits(p); 
			
			return offset;
		}
	}

	if (max_length < UINT32_C(3)) 
		max_length = UINT32_C(3);

	offset = (uint32_t)(lzma_mf_hc_get_matches_spec(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value, distances + offset, max_length) - distances);

	++p->cyclic_buffer_position; 
	p->buffer++; 
	
	if (++p->position == p->position_limit) 
		lzma_mf_check_limits(p); 
	
	return offset;
}


inline static uint32_t lzma_mf_hc_3_zip_get_matches(lzma_match_finder_t* p, uint32_t* distances)
{
	uint32_t offset;
	uint32_t length_limit; 
	uint32_t hv; 
	const uint8_t* cur; 
	uint32_t current_match; 
	
	length_limit = p->length_limit;
	
	if (length_limit < UINT32_C(3)) 
	{ 
		lzma_mf_move_position(p); 
		
		return UINT32_C(0);
	}
	
	cur = p->buffer;
	hv = ((((uint32_t)cur[2]) | (((uint32_t)cur[0]) << 8)) ^ p->crc[cur[1]]) & UINT32_C(0xFFFF);
	current_match = p->hash[hv];
	p->hash[hv] = p->position;

	offset = (uint32_t)(lzma_mf_hc_get_matches_spec(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value, distances, UINT32_C(2)) - distances);
	
	++p->cyclic_buffer_position; 
	p->buffer++; 
	
	if (++p->position == p->position_limit) 
		lzma_mf_check_limits(p); 
	
	return offset;
}


inline static void lzma_mf_bt_2_skip(lzma_match_finder_t* p, uint32_t num)
{
	do
	{
		uint32_t hv, length_limit, current_match;
		const uint8_t* cur; 

		length_limit = p->length_limit;
		
		if (length_limit < UINT32_C(2))
		{ 
			lzma_mf_move_position(p); 
			
			continue; 
		}
		
		cur = p->buffer;
		hv = ((uint32_t)cur[0]) | (((uint32_t)cur[1]) << 8);
		current_match = p->hash[hv];
		p->hash[hv] = p->position;

		lzma_mf_skip_matches_spec(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value); 
		
		++p->cyclic_buffer_position; 
		p->buffer++; 
		
		if (++p->position == p->position_limit) 
			lzma_mf_check_limits(p);
	} 
	while (--num);
}


inline static void lzma_mf_bt_3_zip_skip(lzma_match_finder_t* p, uint32_t num)
{
	do
	{
		uint32_t length_limit; 
		uint32_t hv; 
		const uint8_t* cur; 
		uint32_t current_match; 
		
		length_limit = p->length_limit;
		
		if (length_limit < UINT32_C(3)) 
		{ 
			lzma_mf_move_position(p); 
			
			continue; 
		}
		
		cur = p->buffer;
		hv = ((((uint32_t)cur[2]) | (((uint32_t)cur[0]) << 8)) ^ p->crc[cur[1]]) & UINT32_C(0xFFFF);
		current_match = p->hash[hv];
		p->hash[hv] = p->position;

		lzma_mf_skip_matches_spec(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value); 
		
		++p->cyclic_buffer_position; 
		p->buffer++; 
		
		if (++p->position == p->position_limit) 
			lzma_mf_check_limits(p);
	} 
	while (--num);
}


inline static void lzma_mf_bt_3_skip(lzma_match_finder_t* p, uint32_t num)
{
	do
	{
		uint32_t hc;
		uint32_t* hash;
		uint32_t length_limit; 
		uint32_t hv; 
		const uint8_t* cur; 
		uint32_t current_match; 
		
		length_limit = p->length_limit;
		
		if (length_limit < UINT32_C(3)) 
		{ 
			lzma_mf_move_position(p); 
			
			continue; 
		}
		
		cur = p->buffer;

		uint32_t temp = p->crc[cur[0]] ^ cur[1]; 

		hc = temp & UINT32_C(0x3FF); 
		hv = (temp ^ (((uint32_t)cur[2]) << 8)) & p->hash_mask;
		hash = p->hash;
		current_match = (hash + UINT32_C(0x400))[hv];
		hash[hc] = (hash + UINT32_C(0x400))[hv] = p->position;

		lzma_mf_skip_matches_spec(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value); 
		
		++p->cyclic_buffer_position; 
		p->buffer++; 
		
		if (++p->position == p->position_limit) 
			lzma_mf_check_limits(p);
	}
	while (--num);
}


inline static void lzma_mf_bt_4_skip(lzma_match_finder_t* p, uint32_t num)
{
	do
	{
		uint32_t hv, hc, hd, length_limit, current_match;
		uint32_t* hash; 
		const uint8_t* cur; 

		length_limit = p->length_limit; 
		
		if (length_limit < UINT32_C(4))
		{ 
			lzma_mf_move_position(p); 
			
			continue; 
		}
		
		cur = p->buffer;

		uint32_t temp = p->crc[cur[0]] ^ cur[1]; 
		
		hc = temp & UINT32_C(0x3FF); 
		temp ^= (((uint32_t)cur[2]) << 8); 
		hd = temp & UINT32_C(0xFFFF); 
		hv = (temp ^ (p->crc[cur[3]] << 5)) & p->hash_mask;
		hash = p->hash;
		current_match = (hash + UINT32_C(0x10400))[hv];
		hash[hc] = (hash + UINT32_C(0x400))[hd] = (hash + UINT32_C(0x10400))[hv] = p->position;

		lzma_mf_skip_matches_spec(length_limit, current_match, p->position, p->buffer, p->child, p->cyclic_buffer_position, p->cyclic_buffer_size, p->cut_value); 
		
		++p->cyclic_buffer_position; 
		p->buffer++; 
		
		if (++p->position == p->position_limit) 
			lzma_mf_check_limits(p);
	} 
	while (--num);
}


inline static void lzma_mf_hc_4_skip(lzma_match_finder_t* p, uint32_t num)
{
	do
	{
		uint32_t hv, hc, hd, length_limit, current_match;
		uint32_t* hash;
		const uint8_t* cur; 

		length_limit = p->length_limit;
		
		if (length_limit < UINT32_C(4))
		{ 
			lzma_mf_move_position(p); 
			
			continue; 
		}
		
		cur = p->buffer;

		uint32_t temp = p->crc[cur[0]] ^ cur[1]; 
		
		hc = temp & UINT32_C(0x3FF); 
		temp ^= (((uint32_t)cur[2]) << 8);
		hd = temp & UINT32_C(0xFFFF); 
		hv = (temp ^ (p->crc[cur[3]] << 5)) & p->hash_mask;
		hash = p->hash;

		current_match = (hash + UINT32_C(0x10400))[hv];

		hash[hc] = (hash + UINT32_C(0x400))[hd] = (hash + UINT32_C(0x10400))[hv] = p->position;

		p->child[p->cyclic_buffer_position] = current_match;
		++p->cyclic_buffer_position; 
		p->buffer++; 
		
		if (++p->position == p->position_limit) 
			lzma_mf_check_limits(p);
	} 
	while (--num);
}


void lzma_mf_hc_3_zip_skip(lzma_match_finder_t* p, uint32_t num)
{
	do
	{
		uint32_t hv, length_limit, current_match;
		const uint8_t* cur; 

		length_limit = p->length_limit;
		
		if (length_limit < 3) 
		{ 
			lzma_mf_move_position(p); 
			
			continue; 
		}
		
		cur = p->buffer;
		hv = ((((uint32_t)cur[2]) | (((uint32_t)cur[0]) << 8)) ^ p->crc[cur[1]]) & UINT32_C(0xFFFF);
		current_match = p->hash[hv];
		p->hash[hv] = p->position;
		p->child[p->cyclic_buffer_position] = current_match;

		++p->cyclic_buffer_position; 
		p->buffer++; 
		
		if (++p->position == p->position_limit) lzma_mf_check_limits(p);
	} 
	while (--num);
}


inline static uint32_t lzma_mf_get_matches(lzma_match_finder_t *restrict p, uint32_t* distances)
{
	if (!p->mode_bt)
		return lzma_mf_hc_4_get_matches(p, distances);
	else if (p->hash_byte_count == UINT32_C(2))
		return lzma_mf_bt_2_get_matches(p, distances);
	else if (p->hash_byte_count == UINT32_C(3))
		return lzma_mf_bt_3_get_matches(p, distances);
	else return lzma_mf_bt_4_get_matches(p, distances);
}


inline static void lzma_mf_skip(lzma_match_finder_t *restrict p, uint32_t num)
{
	if (!p->mode_bt) lzma_mf_hc_4_skip(p, num);
	else if (p->hash_byte_count == UINT32_C(2)) lzma_mf_bt_2_skip(p, num);
	else if (p->hash_byte_count == UINT32_C(3)) lzma_mf_bt_3_skip(p, num);
	else lzma_mf_bt_4_skip(p, num);
}


void lzma_encoder_properties_init(lzma_encoder_properties_t* p)
{
	p->level = 5;
	p->dictionary_size = p->mc = UINT32_C(0);
	p->reduce_size = (uint64_t)INT64_C(-1);
	p->lc = p->lp = p->pb = p->algorithm = p->fb = p->mode_bt = p->hash_byte_count = INT32_C(-1);
	p->write_end_mark = UINT32_C(0);
}


void lzma_encoder_properties_normalize(lzma_encoder_properties_t* p)
{
	int32_t level = p->level;

	if (level < INT32_C(0)) 
		level = INT32_C(5);

	p->level = level;

	if (!p->dictionary_size) 
		p->dictionary_size = (level <= 5) ? (UINT32_C(1) << (level * 16)) : (level <= 7) ? UINT32_C(0x2000000) : UINT32_C(0x4000000);

	if (((uint64_t)p->dictionary_size) > p->reduce_size)
	{
		uint32_t i, reduce_size = (uint32_t)p->reduce_size;

		for (i = UINT32_C(11); i <= UINT32_C(30); ++i)
		{
			if (reduce_size <= (UINT32_C(2) << i)) 
			{ 
				p->dictionary_size = (UINT32_C(2) << i); 
				
				break; 
			}

			if (reduce_size <= (UINT32_C(3) << i)) 
			{ 
				p->dictionary_size = (UINT32_C(3) << i); 
				
				break; 
			}
		}
	}

	if (p->lc < 0) p->lc = 3;
	if (p->lp < 0) p->lp = 0;
	if (p->pb < 0) p->pb = 2;

	if (p->algorithm < 0) 
		p->algorithm = (level < 5 ? 0 : 1);

	if (p->fb < 0) 
		p->fb = (level < 7 ? 32 : 64);

	if (p->mode_bt < 0) 
		p->mode_bt = (!p->algorithm) ? 0 : 1;

	if (p->hash_byte_count < 0) 
		p->hash_byte_count = 4;

	if (!p->mc)
		p->mc = (16 + (p->fb >> 1)) >> (p->mode_bt ? 0 : 1);
}


inline static uint32_t lzma_encoder_properties_dictionary_size(lzma_encoder_properties_t* props)
{
	lzma_encoder_properties_normalize(props);
	return props->dictionary_size;
}


inline static void lzma_encoder_fast_pos_init(uint8_t* fast_pos)
{
	size_t i, j, k;

	fast_pos[0] = 0;
	fast_pos[1] = 1;
	fast_pos += 2;

	for (i = UINT64_C(2); i < (UINT64_C(9) + sizeof(size_t) / UINT64_C(2)) * UINT64_C(2); ++i)
	{
		k = (UINT64_C(1) << ((i >> 1) - UINT64_C(1)));

		for (j = UINT64_C(0); j < k; ++j)
			fast_pos[j] = (uint8_t)i;

		fast_pos += k;
	}
}


typedef uint32_t lzma_state_t;


typedef struct lzma_optimal_s
{
	uint32_t cost;
	lzma_state_t state;
	uint8_t prev_a_is_byte;
	int32_t prev_b;
	uint32_t pos_prev_b;
	uint32_t back_prev_b;
	uint32_t pos_prev;
	uint32_t back_prev;
	uint32_t backs[4];
} 
lzma_optimal_t;


typedef struct lzma_len_enc_s
{
	uint16_t choice;
	uint16_t choice_b;
	uint16_t low[0x50];
	uint16_t mid[0x50];
	uint16_t high[0x100];
}
lzma_len_enc_t;


typedef struct lzma_len_cost_enc_s
{
	lzma_len_enc_t p;
	uint32_t table_size;
	uint32_t costs[0x10][0x110];
	uint32_t counters[0x10];
} 
lzma_len_cost_enc_t;


typedef struct lzma_range_enc_s
{
	uint32_t range;
	uint8_t cache;
	uint64_t low;
	uint64_t cache_size;
	uint8_t* buffer;
	uint8_t* buffer_limit;
	uint8_t* buffer_base;
	lzma_seq_out_strm_buf_t* out_stream;
	uint64_t processed;
	lzma_rc_t res;
}
lzma_range_enc_t;


typedef struct lzma_save_state_s
{
	uint16_t* lit_probs;
	uint32_t state;
	uint32_t reps[4];
	uint16_t is_match[0xC][0x10];
	uint16_t is_rep[0xC];
	uint16_t is_rep_g_0[0xC];
	uint16_t is_rep_g_1[0xC];
	uint16_t is_rep_g_2[0xC];
	uint16_t is_rep_0_long[0xC][0x10];
	uint16_t pos_slot_encoder[4][0x40];
	uint16_t pos_encoders[0x72];
	uint16_t pos_align_encoder[0x10];
	lzma_len_cost_enc_t len_enc;
	lzma_len_cost_enc_t rep_len_enc;
} 
lzma_save_state_t;


typedef struct lzma_encoder_s
{
	void* match_finder_obj;
	uint32_t optimum_end_index;
	uint32_t optimum_current_index;
	uint32_t longest_match_length;
	uint32_t pair_count;
	uint32_t available_count;
	uint32_t fast_bytes_count;
	uint32_t additional_offset;
	uint32_t reps[4];
	uint32_t state;
	uint32_t lc, lp, pb;
	uint32_t lp_mask, pb_mask;
	uint32_t lclp;
	uint16_t* lit_probs;
	uint8_t fast_mode;
	uint8_t write_end_mark;
	uint8_t finished;
	uint8_t need_init;
	uint64_t now_pos_64;
	uint32_t match_cost_count;
	uint32_t align_cost_count;
	uint32_t dist_table_size;
	uint32_t dictionary_size;
	lzma_rc_t result;
	lzma_range_enc_t rc;
	lzma_match_finder_t match_finder_base;
	lzma_optimal_t opt[0x1000];
	uint8_t fast_pos[0x2000];
	uint32_t prob_costs[0x80];
	uint32_t matches[0x225];
	uint32_t prob_slot_costs[4][0x40];
	uint32_t distance_costs[4][0x80];
	uint32_t align_costs[0x10];
	uint16_t is_match[0xC][0x10];
	uint16_t is_rep[0xC];
	uint16_t is_rep_g_0[0xC];
	uint16_t is_rep_g_1[0xC];
	uint16_t is_rep_g_2[0xC];
	uint16_t is_rep_0_long[0xC][0x10];
	uint16_t pos_slot_encoder[4][0x40];
	uint16_t pos_encoders[0x72];
	uint16_t pos_align_encoder[0x10];
	lzma_len_cost_enc_t len_enc;
	lzma_len_cost_enc_t rep_len_enc;
	lzma_save_state_t save_state;
	uint8_t pad[0x80];
} 
lzma_encoder_t;


void lzma_encoder_save_state(lzma_encoder_handle_t pp)
{
	lzma_encoder_t* p = (lzma_encoder_t*)pp;
	lzma_save_state_t* dest = &p->save_state;
	uint32_t i;

	dest->len_enc = p->len_enc;
	dest->rep_len_enc = p->rep_len_enc;
	dest->state = p->state;

	for (i = UINT32_C(0); i < UINT32_C(0xC); ++i)
	{
		lzma_memcpy(dest->is_match[i], p->is_match[i], sizeof(p->is_match[i]));
		lzma_memcpy(dest->is_rep_0_long[i], p->is_rep_0_long[i], sizeof(p->is_rep_0_long[i]));
	}

	for (i = UINT32_C(0); i < UINT32_C(4); ++i)
		lzma_memcpy(dest->pos_slot_encoder[i], p->pos_slot_encoder[i], sizeof(p->pos_slot_encoder[i]));

	lzma_memcpy(dest->is_rep, p->is_rep, sizeof(p->is_rep));
	lzma_memcpy(dest->is_rep_g_0, p->is_rep_g_0, sizeof(p->is_rep_g_0));
	lzma_memcpy(dest->is_rep_g_1, p->is_rep_g_1, sizeof(p->is_rep_g_1));
	lzma_memcpy(dest->is_rep_g_2, p->is_rep_g_2, sizeof(p->is_rep_g_2));
	lzma_memcpy(dest->pos_encoders, p->pos_encoders, sizeof(p->pos_encoders));
	lzma_memcpy(dest->pos_align_encoder, p->pos_align_encoder, sizeof(p->pos_align_encoder));
	lzma_memcpy(dest->reps, p->reps, sizeof(p->reps));
	lzma_memcpy(dest->lit_probs, p->lit_probs, (UINT64_C(0x300) << p->lclp) * sizeof(uint16_t));
}


void lzma_encoder_restore_state(lzma_encoder_handle_t pp)
{
	lzma_encoder_t* dest = (lzma_encoder_t*)pp;
	const lzma_save_state_t* p = &dest->save_state;
	uint32_t i;

	dest->len_enc = p->len_enc;
	dest->rep_len_enc = p->rep_len_enc;
	dest->state = p->state;

	for (i = UINT32_C(0); i < UINT32_C(0xC); ++i)
	{
		lzma_memcpy(dest->is_match[i], p->is_match[i], sizeof(p->is_match[i]));
		lzma_memcpy(dest->is_rep_0_long[i], p->is_rep_0_long[i], sizeof(p->is_rep_0_long[i]));
	}

	for (i = UINT32_C(0); i < UINT32_C(4); ++i)
		lzma_memcpy(dest->pos_slot_encoder[i], p->pos_slot_encoder[i], sizeof(p->pos_slot_encoder[i]));

	lzma_memcpy(dest->is_rep, p->is_rep, sizeof(p->is_rep));
	lzma_memcpy(dest->is_rep_g_0, p->is_rep_g_0, sizeof(p->is_rep_g_0));
	lzma_memcpy(dest->is_rep_g_1, p->is_rep_g_1, sizeof(p->is_rep_g_1));
	lzma_memcpy(dest->is_rep_g_2, p->is_rep_g_2, sizeof(p->is_rep_g_2));
	lzma_memcpy(dest->pos_encoders, p->pos_encoders, sizeof(p->pos_encoders));
	lzma_memcpy(dest->pos_align_encoder, p->pos_align_encoder, sizeof(p->pos_align_encoder));
	lzma_memcpy(dest->reps, p->reps, sizeof(p->reps));
	lzma_memcpy(dest->lit_probs, p->lit_probs, (UINT32_C(0x300) << dest->lclp) * sizeof(uint16_t));
}


lzma_rc_t lzma_encoder_set_properties(lzma_encoder_handle_t pp, const lzma_encoder_properties_t* props)
{
	lzma_encoder_t* p = (lzma_encoder_t*)pp;
	lzma_encoder_properties_t properties;

	lzma_memcpy(&properties, props, sizeof(lzma_encoder_properties_t));

	lzma_encoder_properties_normalize(&properties);

	if (properties.lc > 8 || properties.lp > 4 || properties.pb > 4 || properties.dictionary_size > UINT32_C(0x80000000) ||
		properties.dictionary_size > UINT32_C(0x60000000))
		return LZMA_RC_PARAMETER;

	p->dictionary_size = properties.dictionary_size;

	uint32_t fb = properties.fb;

	if (fb < UINT32_C(5))
		fb = UINT32_C(5);

	if (fb > UINT32_C(0x111))
		fb = UINT32_C(0x111);

	p->fast_bytes_count = fb;
	p->lc = properties.lc;
	p->lp = properties.lp;
	p->pb = properties.pb;
	p->fast_mode = !properties.algorithm;
	p->match_finder_base.mode_bt = (properties.mode_bt) ? 1 : 0;

	uint32_t hash_byte_count = UINT32_C(4);

	if (properties.mode_bt)
	{
		if (properties.hash_byte_count < UINT32_C(2))
			hash_byte_count = UINT32_C(2);
		else if (properties.hash_byte_count < UINT32_C(4))
			hash_byte_count = properties.hash_byte_count;
	}

	p->match_finder_base.hash_byte_count = hash_byte_count;
	p->match_finder_base.cut_value = properties.mc;
	p->write_end_mark = properties.write_end_mark;

	return LZMA_RC_OK;
}


void lzma_encoder_set_data_size(lzma_encoder_handle_t p, uint64_t expected_data_size)
{
	((lzma_encoder_t*)p)->match_finder_base.expected_data_size = expected_data_size;
}


inline static void lzma_range_enc_construct(lzma_range_enc_t* p)
{
	p->out_stream = NULL;
	p->buffer_base = NULL;
}


inline static uint8_t lzma_range_enc_allocate(lzma_range_enc_t* p, lzma_allocator_ptr_t allocator)
{
	if (!p->buffer_base)
	{
		p->buffer_base = (uint8_t*)allocator->allocate(allocator->state, UINT64_C(0x10000));

		if (!p->buffer_base)
			return 0;

		p->buffer_limit = p->buffer_base + UINT64_C(0x10000);
	}

	return 1;
}


inline static void lzma_range_enc_free(lzma_range_enc_t* p, lzma_allocator_ptr_t allocator)
{
	allocator->release(allocator, p->buffer_base);
	p->buffer_base = NULL;
}


inline static void lzma_range_enc_init(lzma_range_enc_t* p)
{
	p->low = UINT64_C(0);
	p->range = UINT32_C(0xFFFFFFFF);
	p->cache_size = UINT64_C(1);
	p->cache = 0;
	p->buffer = p->buffer_base;
	p->processed = UINT64_C(0);
	p->res = LZMA_RC_OK;
}


inline static void lzma_range_enc_flush_stream(lzma_range_enc_t* p)
{
	if (p->res != LZMA_RC_OK)
		return;

	size_t num = p->buffer - p->buffer_base;

	if (num != lzma_seq_out_strm_buf_write(p->out_stream, p->buffer_base, num))
		p->res = LZMA_RC_WRITE;

	p->processed += num;
	p->buffer = p->buffer_base;
}


inline static void  lzma_range_enc_shift_low(lzma_range_enc_t* p)
{
	if (p->low < UINT64_C(0xFF000000) || (((uint64_t)p->low) >> 32))
	{
		uint8_t temp = p->cache;

		do
		{
			uint8_t* buffer = p->buffer;

			*buffer++ = (temp + (uint8_t)(p->low >> 32));
			p->buffer = buffer;

			if (buffer == p->buffer_limit)
				lzma_range_enc_flush_stream(p);

			temp = 0xFF;
		} 
		while (--p->cache_size);

		p->cache = (uint8_t)(((uint32_t)p->low) >> 24);
	}

	p->cache_size++;
	p->low = ((uint32_t)p->low) << 8;
}


inline static void lzma_range_enc_flush_data(lzma_range_enc_t* p)
{
	uint32_t i;

	for (i = UINT32_C(0); i < UINT32_C(5); ++i)
		lzma_range_enc_shift_low(p);
}


inline static void lzma_range_enc_encode_direct_bits(lzma_range_enc_t* p, uint32_t value, uint32_t bit_count)
{
	do
	{
		p->range >>= 1;
		p->low += p->range & (UINT32_C(0) - ((value >> --bit_count) & UINT32_C(1)));

		if (p->range < UINT32_C(0x1000000))
		{
			p->range <<= 8;
			lzma_range_enc_shift_low(p);
		}
	}
	while (bit_count);
}


inline static void lzma_range_enc_encode_bit(lzma_range_enc_t* p, uint16_t *prob, uint32_t symbol)
{
	uint32_t temp_a = *prob;
	uint32_t new_bound = (p->range >> 11) * temp_a;

	if (!symbol)
	{
		p->range = new_bound;
		temp_a += (UINT32_C(0x800) - temp_a) >> 5;
	}
	else
	{
		p->low += new_bound;
		p->range -= new_bound;
		temp_a -= temp_a >> 5;
	}

	*prob = (uint16_t)temp_a;

	if (p->range < UINT32_C(0x1000000))
	{
		p->range <<= 8;

		lzma_range_enc_shift_low(p);
	}
}


inline static void lzma_lit_enc_encode(lzma_range_enc_t* p, uint16_t* probabilities, uint32_t symbol)
{
	symbol |= UINT32_C(0x100);

	do
	{
		lzma_range_enc_encode_bit(p, probabilities + (symbol >> 8), (symbol >> 7) & UINT32_C(1));

		symbol <<= 1;
	}
	while (symbol < UINT32_C(0x10000));
}


inline static void lzma_lit_enc_encode_matched(lzma_range_enc_t* p, uint16_t* probabilities, uint32_t symbol, uint32_t match_byte)
{
	uint32_t offs = UINT32_C(0x100);

	symbol |= UINT32_C(0x100);

	do
	{
		match_byte <<= 1;

		lzma_range_enc_encode_bit(p, probabilities + (offs + (match_byte & offs) + (symbol >> 8)), (symbol >> 7) & UINT32_C(1));

		symbol <<= 1;

		offs &= ~(match_byte ^ symbol);
	} 
	while (symbol < UINT32_C(0x10000));
}


inline static void lzma_encoder_init_cost_tables(uint32_t* prob_costs)
{
	uint32_t i;

	for (i = UINT32_C(8); i < UINT32_C(0x800); i += UINT32_C(0x10))
	{
		uint32_t j, w = i, bit_count = UINT32_C(0);

		for (j = UINT32_C(0); j < UINT32_C(4); ++j)
		{
			w = w * w;
			bit_count <<= 1;

			while (w >= UINT32_C(0x10000))
			{
				w >>= 1;
				bit_count++;
			}
		}

		prob_costs[i >> 4] = UINT32_C(0xA1) - bit_count;
	}
}


inline static uint32_t lzma_lit_enc_get_cost(const uint16_t* probabilities, uint32_t symbol, const uint32_t* prob_costs)
{
	uint32_t cost = UINT32_C(0);

	symbol |= UINT32_C(0x100);

	do
	{
		cost += prob_costs[((probabilities[symbol >> 8]) ^ ((-((int32_t)((symbol >> 7) & UINT32_C(1)))) & UINT32_C(0x7FF))) >> 4];
		symbol <<= 1;
	}
	while (symbol < UINT32_C(0x10000));

	return cost;
}


inline static uint32_t lzma_lit_enc_get_cost_matched(const uint16_t* probabilities, uint32_t symbol, uint32_t match_byte, const uint32_t* prob_costs)
{
	uint32_t cost = UINT32_C(0), offs = UINT32_C(0x100);

	symbol |= UINT32_C(0x100);

	do
	{
		match_byte <<= 1;
		cost += prob_costs[((probabilities[offs + (match_byte & offs) + (symbol >> 8)]) ^ ((-((int32_t)((symbol >> 7) & UINT32_C(1)))) & UINT32_C(0x7FF))) >> 4];
		symbol <<= 1;
		offs &= ~(match_byte ^ symbol);
	} 
	while (symbol < UINT32_C(0x10000));

	return cost;
}


inline static void lzma_rc_tree_encode(lzma_range_enc_t* rc, uint16_t* probabilities, uint32_t bit_levels_count, uint32_t symbol)
{
	uint32_t i, m = UINT32_C(1);

	for (i = bit_levels_count; i;)
	{
		i--;

		uint32_t bit = (symbol >> i) & UINT32_C(1);

		lzma_range_enc_encode_bit(rc, probabilities + m, bit);

		m = (m << 1) | bit;
	}
}


inline static void lzma_rc_tree_reverse_encode(lzma_range_enc_t* rc, uint16_t* probabilities, uint32_t bit_levels_count, uint32_t symbol)
{
	uint32_t i, m = UINT32_C(1);

	for (i = UINT32_C(0); i < bit_levels_count; ++i)
	{
		uint32_t bit = symbol & UINT32_C(1);

		lzma_range_enc_encode_bit(rc, probabilities + m, bit);

		m = (m << 1) | bit;

		symbol >>= 1;
	}
}


inline static uint32_t lzma_rc_tree_get_cost(const uint16_t* probabilities, int32_t bit_levels_count, uint32_t symbol, const uint32_t* prob_costs)
{
	uint32_t cost = UINT32_C(0);
	symbol |= (UINT32_C(1) << bit_levels_count);

	while (symbol != UINT32_C(1))
	{
		cost += prob_costs[((probabilities[symbol >> 1]) ^ ((-((int32_t)(symbol & UINT32_C(1)))) & UINT32_C(0x7FF))) >> 4];
		symbol >>= 1;
	}

	return cost;
}

inline static uint32_t lzma_rc_tree_reverse_get_cost(const uint16_t* probabilities, uint32_t bit_levels_count, uint32_t symbol, const uint32_t* prob_costs)
{
	uint32_t i, m = UINT32_C(1), cost = UINT32_C(0);

	for (i = bit_levels_count; i; --i)
	{
		uint32_t bit = symbol & UINT32_C(1);

		symbol >>= 1;

		cost += prob_costs[((probabilities[m]) ^ ((-((int32_t)bit)) & UINT32_C(0x7FF))) >> 4];

		m = (m << 1) | bit;
	}

	return cost;
}


inline static void lzma_len_enc_init(lzma_len_enc_t* p)
{
	uint32_t i;

	p->choice = p->choice_b = UINT16_C(0x400);

	for (i = UINT32_C(0); i < UINT32_C(0x80); ++i)
		p->low[i] = UINT16_C(0x400);

	for (i = UINT32_C(0); i < UINT32_C(0x80); ++i)
		p->mid[i] = UINT16_C(0x400);

	for (i = UINT32_C(0); i < UINT32_C(0x100); ++i)
		p->high[i] = UINT16_C(0x400);
}


inline static void lzma_len_enc_encode(lzma_len_enc_t* p, lzma_range_enc_t* rc, uint32_t symbol, uint32_t pos_state)
{
	if (symbol < UINT32_C(8))
	{
		lzma_range_enc_encode_bit(rc, &p->choice, UINT32_C(0));
		lzma_rc_tree_encode(rc, p->low + (pos_state << 3), UINT32_C(3), symbol);
	}
	else
	{
		lzma_range_enc_encode_bit(rc, &p->choice, UINT32_C(1));

		if (symbol < UINT32_C(0x10))
		{
			lzma_range_enc_encode_bit(rc, &p->choice_b, UINT32_C(0));
			lzma_rc_tree_encode(rc, p->mid + (pos_state << 3), UINT32_C(3), symbol - UINT32_C(8));
		}
		else
		{
			lzma_range_enc_encode_bit(rc, &p->choice_b, UINT32_C(1));
			lzma_rc_tree_encode(rc, p->high, UINT32_C(8), symbol - UINT32_C(8) - UINT32_C(8));
		}
	}
}


inline static void lzma_len_enc_set_costs(lzma_len_enc_t* p, uint32_t pos_state, uint32_t symbol_count, uint32_t* costs, const uint32_t* prob_costs)
{
	uint32_t i;

	uint32_t aa = prob_costs[p->choice >> 4];
	uint32_t ab = prob_costs[(p->choice ^ UINT16_C(0x7FF)) >> 4];
	uint32_t ba = ab + prob_costs[p->choice_b >> 4];
	uint32_t bb = ab + prob_costs[(p->choice_b ^ UINT16_C(0x7FF)) >> 4];

	for (i = UINT32_C(0); i < UINT32_C(8); ++i)
	{
		if (i >= symbol_count)
			return;

		costs[i] = aa + lzma_rc_tree_get_cost(p->low + (pos_state << 3), 3, i, prob_costs);
	}

	for (; i < UINT32_C(0x10); ++i)
	{
		if (i >= symbol_count)
			return;

		costs[i] = ba + lzma_rc_tree_get_cost(p->mid + (pos_state << 3), 3, i - UINT32_C(8), prob_costs);
	}

	for (; i < symbol_count; ++i)
		costs[i] = bb + lzma_rc_tree_get_cost(p->high, 8, i - UINT32_C(8) - UINT32_C(8), prob_costs);
}


inline static void lzma_len_cost_enc_update_tab(lzma_len_cost_enc_t* p, uint32_t pos_state, const uint32_t* prob_costs)
{
	lzma_len_enc_set_costs(&p->p, pos_state, p->table_size, p->costs[pos_state], prob_costs);

	p->counters[pos_state] = p->table_size;
}


inline static void lzma_len_cost_enc_update_tabs(lzma_len_cost_enc_t* p, uint32_t nr_pos_states, const uint32_t* prob_costs)
{
	uint32_t i;

	for (i = UINT32_C(0); i < nr_pos_states; ++i)
		lzma_len_cost_enc_update_tab(p, i, prob_costs);
}


inline static void lzma_len_enc_encode_ex(lzma_len_cost_enc_t* p, lzma_range_enc_t* rc, uint32_t symbol, uint32_t pos_state, uint8_t update_cost, const uint32_t* prob_costs)
{
	lzma_len_enc_encode(&p->p, rc, symbol, pos_state);

	if (update_cost)
		if (--p->counters[pos_state] == UINT32_C(0))
			lzma_len_cost_enc_update_tab(p, pos_state, prob_costs);
}


inline static void lzma_move_pos(lzma_encoder_t* p, uint32_t num)
{
	if (num)
	{
		p->additional_offset += num;
		lzma_mf_skip(p->match_finder_obj, num);
	}
}


inline static uint32_t lzma_read_match_dists(lzma_encoder_t* p, uint32_t* nr_dist_pairs_res)
{
	uint32_t len_res = UINT32_C(0), pair_count;

	p->available_count = lzma_mf_get_num_available_bytes(p->match_finder_obj);
	pair_count = lzma_mf_get_matches(p->match_finder_obj, p->matches);

	if (pair_count > UINT32_C(0))
	{
		len_res = p->matches[pair_count - UINT32_C(2)];

		if (len_res == p->fast_bytes_count)
		{
			uint32_t available_count = p->available_count;

			if (available_count > UINT32_C(0x111))
				available_count = UINT32_C(0x111);

			const uint8_t* pby_cur = lzma_mf_get_ptr_to_current_position(p->match_finder_obj) - UINT32_C(1);
			const uint8_t* pby_lim = pby_cur + available_count;
			const uint8_t* pby = pby_cur + len_res;

			ptrdiff_t diff = INT64_C(-1) - p->matches[pair_count - UINT32_C(1)];

			for (; pby != pby_lim && *pby == pby[diff]; ++pby);

			len_res = (uint32_t)(pby - pby_cur);
		}
	}

	p->additional_offset++;
	*nr_dist_pairs_res = pair_count;

	return len_res;
}


inline static uint32_t lzma_get_rep_len_a_cost(lzma_encoder_t* p, uint32_t state, uint32_t pos_state)
{
	return p->prob_costs[p->is_rep_g_0[state] >> 4] + p->prob_costs[p->is_rep_0_long[state][pos_state] >> 4];
}


inline static uint32_t lzma_get_pure_rep_cost(lzma_encoder_t* p, uint32_t rep_index, uint32_t state, uint32_t pos_state)
{
	uint32_t cost;

	if (rep_index == UINT32_C(0))
	{
		cost = p->prob_costs[p->is_rep_g_0[state] >> 4];
		cost += p->prob_costs[(p->is_rep_0_long[state][pos_state] ^ UINT16_C(0x7FF)) >> 4];
	}
	else
	{
		cost = p->prob_costs[(p->is_rep_g_0[state] ^ UINT16_C(0x7FF)) >> 4];

		if (rep_index == UINT32_C(1))
			cost += p->prob_costs[p->is_rep_g_1[state] >> 4];
		else
		{
			cost += p->prob_costs[(p->is_rep_g_1[state] ^ UINT16_C(0x7FF)) >> 4];
			cost += p->prob_costs[(p->is_rep_g_2[state] ^ (-(int32_t)(rep_index - UINT32_C(2))) & UINT16_C(0x7FF)) >> 4];
		}
	}

	return cost;
}


inline static uint32_t lzma_get_rep_cost(lzma_encoder_t* p, uint32_t rep_index, uint32_t len, uint32_t state, uint32_t pos_state)
{
	return p->rep_len_enc.costs[pos_state][len - UINT32_C(2)] + lzma_get_pure_rep_cost(p, rep_index, state, pos_state);
}


inline static uint32_t lzma_backward(lzma_encoder_t* p, uint32_t* back_res, uint32_t cur)
{
	uint32_t mem_pos = p->opt[cur].pos_prev;
	uint32_t mem_back = p->opt[cur].back_prev;

	p->optimum_end_index = cur;

	do
	{
		if (p->opt[cur].prev_a_is_byte)
		{
			p->opt[mem_pos].back_prev = (uint32_t)-1;
			p->opt[mem_pos].prev_a_is_byte = 0;
			p->opt[mem_pos].pos_prev = mem_pos - UINT32_C(1);

			if (p->opt[cur].prev_b)
			{
				p->opt[mem_pos - UINT32_C(1)].prev_a_is_byte = 0;
				p->opt[mem_pos - UINT32_C(1)].pos_prev = p->opt[cur].pos_prev_b;
				p->opt[mem_pos - UINT32_C(1)].back_prev = p->opt[cur].back_prev_b;
			}
		}

		uint32_t pos_prev = mem_pos, cur_back = mem_back;

		mem_back = p->opt[pos_prev].back_prev;
		mem_pos = p->opt[pos_prev].pos_prev;

		p->opt[pos_prev].back_prev = cur_back;
		p->opt[pos_prev].pos_prev = cur;
		cur = pos_prev;
	}
	while (cur);

	*back_res = p->opt[0].back_prev;
	p->optimum_current_index = p->opt[0].pos_prev;

	return p->optimum_current_index;
}


inline static uint32_t lzma_tab_match_next_states(register uint32_t i)
{
	return (i < UINT32_C(7)) ? UINT32_C(7) : UINT32_C(10);
}


inline static uint32_t lzma_tab_replace_next_states(register uint32_t i)
{
	return (i < UINT32_C(7)) ? UINT32_C(8) : UINT32_C(11);
}


inline static uint32_t lzma_tab_short_replace_next_states(register uint32_t i)
{
	return (i < UINT32_C(7)) ? UINT32_C(9) : UINT32_C(11);
}


inline static uint32_t lzma_tab_literal_next_states(register uint32_t i)
{
	if (i < UINT32_C(4)) return UINT32_C(0);
	else if (i < UINT32_C(10)) return i - UINT32_C(3);
	return i - UINT32_C(6);
}


inline static uint32_t lzma_get_optimum(lzma_encoder_t* p, uint32_t position, uint32_t* back_res)
{
	uint32_t len_end, cur, reps[4], rep_lens[4];
	uint32_t* matches;
	uint32_t i, len, available_count, main_len, pair_count, rep_max_index, pos_state;
	uint32_t match_cost, rep_match_cost, normal_match_cost;
	uint8_t cur_byte, match_byte;
	const uint8_t* data;

	if (p->optimum_end_index != p->optimum_current_index)
	{
		const lzma_optimal_t* opt = &p->opt[p->optimum_current_index];
		uint32_t len_res = opt->pos_prev - p->optimum_current_index;

		*back_res = opt->back_prev;
		p->optimum_current_index = opt->pos_prev;

		return len_res;
	}

	p->optimum_current_index = p->optimum_end_index = UINT32_C(0);

	if (!p->additional_offset)
		main_len = lzma_read_match_dists(p, &pair_count);
	else
	{
		main_len = p->longest_match_length;
		pair_count = p->pair_count;
	}

	available_count = p->available_count;

	if (available_count < 2)
	{
		*back_res = (uint32_t)-1;
		return UINT32_C(1);
	}

	if (available_count > UINT32_C(0x111))
		available_count = UINT32_C(0x111);

	data = lzma_mf_get_ptr_to_current_position(p->match_finder_obj) - UINT32_C(1);
	rep_max_index = UINT32_C(0);

	for (i = UINT32_C(0); i < UINT32_C(4); ++i)
	{
		reps[i] = p->reps[i];

		const uint8_t* data_b = data - reps[i] - UINT32_C(1);

		if (data[0] != data_b[0] || data[1] != data_b[1])
		{
			rep_lens[i] = UINT32_C(0);

			continue;
		}

		uint32_t len_test;

		for (len_test = UINT32_C(2); len_test < available_count && data[len_test] == data_b[len_test]; ++len_test);

		rep_lens[i] = len_test;

		if (len_test > rep_lens[rep_max_index])
			rep_max_index = i;
	}

	if (rep_lens[rep_max_index] >= p->fast_bytes_count)
	{
		*back_res = rep_max_index;

		uint32_t len_res = rep_lens[rep_max_index];

		lzma_move_pos(p, len_res - UINT32_C(1));

		return len_res;
	}

	matches = p->matches;

	if (main_len >= p->fast_bytes_count)
	{
		*back_res = matches[pair_count - UINT32_C(1)] + UINT32_C(4);

		lzma_move_pos(p, main_len - UINT32_C(1));

		return main_len;
	}

	cur_byte = *data;
	match_byte = *(data - (reps[0] + UINT32_C(1)));

	if (main_len < UINT32_C(2) && cur_byte != match_byte && rep_lens[rep_max_index] < UINT32_C(2))
	{
		*back_res = (uint32_t)-1;

		return UINT32_C(1);
	}

	p->opt[0].state = p->state;
	pos_state = (position & p->pb_mask);

	const uint16_t* probs = (p->lit_probs + ((((position)& p->lp_mask) << p->lc) + ((*(data - 1)) >> (8 - p->lc))) * UINT32_C(0x300));

	p->opt[1].cost = p->prob_costs[(p->is_match[p->state][pos_state]) >> 4] + !(p->state < UINT32_C(7)) ?
		lzma_lit_enc_get_cost_matched(probs, cur_byte, match_byte, p->prob_costs) :
		lzma_lit_enc_get_cost(probs, cur_byte, p->prob_costs);

	p->opt[1].back_prev = (uint32_t)-1;
	p->opt[1].prev_a_is_byte = 0;

	match_cost = p->prob_costs[(p->is_match[p->state][pos_state] ^ UINT16_C(0x7FF)) >> 4];
	rep_match_cost = match_cost + p->prob_costs[(p->is_rep[p->state] ^ UINT16_C(0x7FF)) >> 4];

	if (match_byte == cur_byte)
	{
		uint32_t short_rep_cost = rep_match_cost + lzma_get_rep_len_a_cost(p, p->state, pos_state);

		if (short_rep_cost < p->opt[1].cost)
		{
			p->opt[1].cost = short_rep_cost;
			p->opt[1].back_prev = UINT32_C(0);
			p->opt[1].prev_a_is_byte = 0;
		}
	}

	len_end = (main_len >= rep_lens[rep_max_index]) ? main_len : rep_lens[rep_max_index];

	if (len_end < UINT32_C(2))
	{
		*back_res = p->opt[1].back_prev;

		return UINT32_C(1);
	}

	p->opt[1].pos_prev = UINT32_C(0);

	for (i = UINT32_C(0); i < UINT32_C(4); ++i)
		p->opt[0].backs[i] = reps[i];

	len = len_end;

	do p->opt[len--].cost = UINT32_C(0x40000000);
	while (len >= UINT32_C(2));

	for (i = UINT32_C(0); i < UINT32_C(4); ++i)
	{
		uint32_t cost, rep_len = rep_lens[i];

		if (rep_len < UINT32_C(2))
			continue;

		cost = rep_match_cost + lzma_get_pure_rep_cost(p, i, p->state, pos_state);

		do
		{
			uint32_t cur_and_len_cost = cost + p->rep_len_enc.costs[pos_state][rep_len - UINT32_C(2)];
			lzma_optimal_t* opt = &p->opt[rep_len];

			if (cur_and_len_cost < opt->cost)
			{
				opt->cost = cur_and_len_cost;
				opt->pos_prev = UINT32_C(0);
				opt->back_prev = i;
				opt->prev_a_is_byte = 0;
			}
		}
		while (--rep_len >= UINT32_C(2));
	}

	normal_match_cost = match_cost + p->prob_costs[p->is_rep[p->state] >> 4];

	len = (rep_lens[0] >= UINT32_C(2)) ? rep_lens[0] + UINT32_C(1) : UINT32_C(2);

	if (len <= main_len)
	{
		uint32_t offs = UINT32_C(0);

		while (len > matches[offs]) 
			offs += UINT32_C(2);

		for (;; ++len)
		{
			lzma_optimal_t* opt;

			uint32_t distance = matches[offs + UINT32_C(1)];
			uint32_t cur_and_len_cost = normal_match_cost + p->len_enc.costs[pos_state][len - UINT32_C(2)];
			uint32_t len_to_pos_state = (len < UINT32_C(5)) ? len - UINT32_C(2) : UINT32_C(3);

			if (distance < UINT32_C(0x80))
				cur_and_len_cost += p->distance_costs[len_to_pos_state][distance];
			else
			{
				uint32_t slot;
				uint32_t t = (distance < (UINT32_C(1) << ((UINT32_C(9) + sizeof(size_t) / UINT32_C(2)) + UINT32_C(6)))) ?
					UINT32_C(6) : UINT32_C(6) + (UINT32_C(9) + sizeof(size_t) / UINT32_C(2)) - UINT32_C(1);

				slot = p->fast_pos[distance >> t] + (t * UINT32_C(2));
				cur_and_len_cost += p->align_costs[distance & UINT32_C(0xF)] + p->prob_slot_costs[len_to_pos_state][slot];
			}

			opt = &p->opt[len];

			if (cur_and_len_cost < opt->cost)
			{
				opt->cost = cur_and_len_cost;
				opt->pos_prev = UINT32_C(0);
				opt->back_prev = distance + UINT32_C(4);
				opt->prev_a_is_byte = 0;
			}

			if (len == matches[offs])
			{
				offs += UINT32_C(2);

				if (offs == pair_count)
					break;
			}
		}
	}

	cur = UINT32_C(0);

	for (;;)
	{
		uint32_t available_count, num_aval_full, new_len, pair_count, pos_prev, state, pos_state, start_len;
		uint32_t cur_cost, cur_and_1_cost, match_cost, rep_match_cost;
		uint8_t next_is_byte, cur_byte, match_byte;
		const uint8_t* data;
		lzma_optimal_t *cur_opt, *next_opt;

		cur++;

		if (cur == len_end)
			return lzma_backward(p, back_res, cur);

		new_len = lzma_read_match_dists(p, &pair_count);

		if (new_len >= p->fast_bytes_count)
		{
			p->pair_count = pair_count;
			p->longest_match_length = new_len;

			return lzma_backward(p, back_res, cur);
		}

		position++;
		cur_opt = &p->opt[cur];
		pos_prev = cur_opt->pos_prev;

		if (cur_opt->prev_a_is_byte)
		{
			pos_prev--;

			if (cur_opt->prev_b)
			{
				state = p->opt[cur_opt->pos_prev_b].state;

				if (cur_opt->back_prev_b < UINT32_C(4))
					state = lzma_tab_replace_next_states(state);
				else state = lzma_tab_match_next_states(state);
			}
			else state = p->opt[pos_prev].state;

			state = lzma_tab_literal_next_states(state);
		}
		else state = p->opt[pos_prev].state;

		if (pos_prev == cur - UINT32_C(1))
		{
			if (!cur_opt->back_prev)
				state = lzma_tab_short_replace_next_states(state);
			else state = lzma_tab_literal_next_states(state);
		}
		else
		{
			uint32_t pos;
			const lzma_optimal_t* prev_opt;

			if (cur_opt->prev_a_is_byte && cur_opt->prev_b)
			{
				pos_prev = cur_opt->pos_prev_b;
				pos = cur_opt->back_prev_b;
				state = lzma_tab_replace_next_states(state);
			}
			else
			{
				pos = cur_opt->back_prev;

				if (pos < UINT32_C(4))
					state = lzma_tab_replace_next_states(state);
				else state = lzma_tab_match_next_states(state);
			}

			prev_opt = &p->opt[pos_prev];

			if (pos < UINT32_C(4))
			{
				uint32_t i;

				reps[0] = prev_opt->backs[pos];

				for (i = UINT32_C(1); i <= pos; ++i)
					reps[i] = prev_opt->backs[i - UINT32_C(1)];

				for (; i < UINT32_C(4); ++i)
					reps[i] = prev_opt->backs[i];
			}
			else
			{
				uint32_t i;

				reps[0] = pos - UINT32_C(4);

				for (i = UINT32_C(1); i < UINT32_C(4); ++i)
					reps[i] = prev_opt->backs[i - UINT32_C(1)];
			}
		}

		cur_opt->state = state;

		cur_opt->backs[0] = reps[0];
		cur_opt->backs[1] = reps[1];
		cur_opt->backs[2] = reps[2];
		cur_opt->backs[3] = reps[3];

		cur_cost = cur_opt->cost;
		next_is_byte = 0;

		data = lzma_mf_get_ptr_to_current_position(p->match_finder_obj) - UINT32_C(1);

		cur_byte = *data;
		match_byte = *(data - (reps[0] + UINT32_C(1)));
		pos_state = (position & p->pb_mask);
		cur_and_1_cost = cur_cost + p->prob_costs[p->is_match[state][pos_state] >> 4];

		const uint16_t* probs = (p->lit_probs + ((((position)& p->lp_mask) << p->lc) + 
			((*(data - UINT32_C(1))) >> (UINT32_C(8) - p->lc))) * UINT32_C(0x300));

		cur_and_1_cost += !(state < UINT32_C(7)) ? 
			lzma_lit_enc_get_cost_matched(probs, cur_byte, match_byte, p->prob_costs) :
			lzma_lit_enc_get_cost(probs, cur_byte, p->prob_costs);

		next_opt = &p->opt[cur + UINT32_C(1)];

		if (cur_and_1_cost < next_opt->cost)
		{
			next_opt->cost = cur_and_1_cost;
			next_opt->pos_prev = cur;
			next_opt->back_prev = (uint32_t)-1;
			next_opt->prev_a_is_byte = UINT32_C(0);

			next_is_byte = 1;
		}

		match_cost = cur_cost + p->prob_costs[(p->is_match[state][pos_state] ^ UINT16_C(0x7FF)) >> 4];
		rep_match_cost = match_cost + p->prob_costs[(p->is_rep[state] ^ UINT16_C(0x7FF)) >> 4];

		if (match_byte == cur_byte && !(next_opt->pos_prev < cur && !next_opt->back_prev))
		{
			uint32_t short_rep_cost = rep_match_cost + lzma_get_rep_len_a_cost(p, state, pos_state);

			if (short_rep_cost <= next_opt->cost)
			{
				next_opt->cost = short_rep_cost;
				next_opt->pos_prev = cur;
				next_opt->back_prev = UINT32_C(0);
				next_opt->prev_a_is_byte = 0;

				next_is_byte = 1;
			}
		}

		num_aval_full = p->available_count;

		uint32_t temp = UINT32_C(0xFFF) - cur;

		if (temp < num_aval_full)
			num_aval_full = temp;

		if (num_aval_full < UINT32_C(2))
			continue;

		available_count = (num_aval_full <= p->fast_bytes_count ? num_aval_full : p->fast_bytes_count);

		if (!next_is_byte && match_byte != cur_byte)
		{
			uint32_t temp, len_test_b, limit = p->fast_bytes_count + UINT32_C(1);
			const uint8_t* data_b = data - reps[0] - UINT32_C(1);

			if (limit > num_aval_full)
				limit = num_aval_full;

			for (temp = UINT32_C(1); temp < limit && data[temp] == data_b[temp]; ++temp);

			len_test_b = temp - UINT32_C(1);

			if (len_test_b >= UINT32_C(2))
			{
				uint32_t state_b = lzma_tab_literal_next_states(state);
				uint32_t pos_state_next = (position + UINT32_C(1)) & p->pb_mask;

				uint32_t next_rep_match_cost = cur_and_1_cost +
					p->prob_costs[(p->is_match[state_b][pos_state_next] ^ UINT16_C(0x7FF)) >> 4] +
					p->prob_costs[(p->is_rep[state_b] ^ UINT16_C(0x7FF)) >> 4];

				uint32_t cur_and_len_cost;
				lzma_optimal_t* opt;
				uint32_t offset = cur + UINT32_C(1) + len_test_b;

				while (len_end < offset)
					p->opt[++len_end].cost = UINT32_C(0x40000000);

				cur_and_len_cost = next_rep_match_cost + lzma_get_rep_cost(p, UINT32_C(0), len_test_b, state_b, pos_state_next);

				opt = &p->opt[offset];

				if (cur_and_len_cost < opt->cost)
				{
					opt->cost = cur_and_len_cost;
					opt->pos_prev = cur + UINT32_C(1);
					opt->back_prev = 0;
					opt->prev_a_is_byte = 1;
					opt->prev_b = 0;
				}
			}
		}

		start_len = UINT32_C(2);

		uint32_t rep_index;

		for (rep_index = UINT32_C(0); rep_index < UINT32_C(4); ++rep_index)
		{
			uint32_t len_test, len_test_temp, cost;
			const uint8_t* data_b = data - reps[rep_index] - UINT32_C(1);

			if (data[0] != data_b[0] || data[1] != data_b[1])
				continue;

			for (len_test = UINT32_C(2); len_test < available_count && data[len_test] == data_b[len_test]; ++len_test);

			while (len_end < cur + len_test)
				p->opt[++len_end].cost = UINT32_C(0x40000000);

			len_test_temp = len_test;
			cost = rep_match_cost + lzma_get_pure_rep_cost(p, rep_index, state, pos_state);

			do
			{
				uint32_t cur_and_len_cost = cost + p->rep_len_enc.costs[pos_state][len_test - UINT32_C(2)];
				lzma_optimal_t* opt = &p->opt[cur + len_test];

				if (cur_and_len_cost < opt->cost)
				{
					opt->cost = cur_and_len_cost;
					opt->pos_prev = cur;
					opt->back_prev = rep_index;
					opt->prev_a_is_byte = 0;
				}
			}
			while (--len_test >= UINT32_C(2));

			len_test = len_test_temp;

			if (rep_index == UINT32_C(0))
				start_len = len_test + UINT32_C(1);

			uint32_t len_test_b = len_test + UINT32_C(1);
			uint32_t limit = len_test_b + p->fast_bytes_count;

			if (limit > num_aval_full)
				limit = num_aval_full;

			for (; len_test_b < limit && data[len_test_b] == data_b[len_test_b]; ++len_test_b);

			len_test_b -= len_test + UINT32_C(1);

			if (len_test_b >= UINT32_C(2))
			{
				uint32_t next_rep_match_cost;
				uint32_t state_b = lzma_tab_replace_next_states(state);
				uint32_t pos_state_next = (position + len_test) & p->pb_mask;

				uint32_t cur_and_len_byte_cost =
					cost + p->rep_len_enc.costs[pos_state][len_test - UINT32_C(2)] +
					p->prob_costs[p->is_match[state_b][pos_state_next] >> 4] +
					lzma_lit_enc_get_cost_matched((p->lit_probs + ((((position + len_test) & p->lp_mask) << p->lc) + (data[len_test - UINT32_C(1)] >> (UINT32_C(8) - p->lc))) * UINT32_C(0x300)),
						data[len_test], data_b[len_test], p->prob_costs);

				state_b = lzma_tab_literal_next_states(state_b);
				pos_state_next = (position + len_test + UINT32_C(1)) & p->pb_mask;

				next_rep_match_cost = cur_and_len_byte_cost +
					p->prob_costs[(p->is_match[state_b][pos_state_next] ^ UINT16_C(0x7FF)) >> 4] +
					p->prob_costs[((p->is_rep[state_b]) ^ UINT16_C(0x7FF)) >> 4];

				uint32_t cur_and_len_cost;
				lzma_optimal_t* opt;
				uint32_t offset = cur + len_test + UINT32_C(1) + len_test_b;

				while (len_end < offset)
					p->opt[++len_end].cost = UINT32_C(0x40000000);

				cur_and_len_cost = next_rep_match_cost + lzma_get_rep_cost(p, UINT32_C(0), len_test_b, state_b, pos_state_next);
				opt = &p->opt[offset];

				if (cur_and_len_cost < opt->cost)
				{
					opt->cost = cur_and_len_cost;
					opt->pos_prev = cur + len_test + UINT32_C(1);
					opt->back_prev = UINT32_C(0);
					opt->prev_a_is_byte = 1;
					opt->prev_b = 1;
					opt->pos_prev_b = cur;
					opt->back_prev_b = rep_index;
				}
			}
		}

		if (new_len > available_count)
		{
			new_len = available_count;

			for (pair_count = UINT32_C(0); new_len > matches[pair_count]; pair_count += UINT32_C(2));

			matches[pair_count] = new_len;
			pair_count += UINT32_C(2);
		}

		if (new_len >= start_len)
		{
			uint32_t normal_match_cost = match_cost + p->prob_costs[p->is_rep[state] >> 4];
			uint32_t offs, cur_back, pos_slot;
			uint32_t len_test;

			while (len_end < cur + new_len)
				p->opt[++len_end].cost = UINT32_C(0x40000000);

			offs = UINT32_C(0);

			while (start_len > matches[offs])
				offs += UINT32_C(2);

			cur_back = matches[offs + UINT32_C(1)];

			uint32_t zz = (cur_back < (UINT32_C(1) << ((UINT32_C(9) + sizeof(size_t) / UINT32_C(2)) + UINT32_C(6)))) ? 
				UINT32_C(6) : UINT32_C(6) + (UINT32_C(9) + sizeof(size_t) / UINT32_C(2)) - UINT32_C(1);

			pos_slot = p->fast_pos[cur_back >> zz] + (zz * UINT32_C(2));

			for (len_test = start_len; ; ++len_test)
			{
				uint32_t cur_and_len_cost = normal_match_cost + p->len_enc.costs[pos_state][len_test - UINT32_C(2)];
				uint32_t len_to_pos_state = (len_test < UINT32_C(5)) ? len_test - UINT32_C(2) : UINT32_C(3);
				lzma_optimal_t* opt;

				if (cur_back < UINT32_C(0x80))
					cur_and_len_cost += p->distance_costs[len_to_pos_state][cur_back];
				else cur_and_len_cost += p->prob_slot_costs[len_to_pos_state][pos_slot] + p->align_costs[cur_back & UINT32_C(0xF)];

				opt = &p->opt[cur + len_test];

				if (cur_and_len_cost < opt->cost)
				{
					opt->cost = cur_and_len_cost;
					opt->pos_prev = cur;
					opt->back_prev = cur_back + UINT32_C(4);
					opt->prev_a_is_byte = 0;
				}

				if (len_test == matches[offs])
				{
					const uint8_t* data_b = data - cur_back - UINT32_C(1);
					uint32_t len_test_b = len_test + UINT32_C(1);
					uint32_t limit = len_test_b + p->fast_bytes_count;

					if (limit > num_aval_full)
						limit = num_aval_full;

					for (; len_test_b < limit && data[len_test_b] == data_b[len_test_b]; ++len_test_b);

					len_test_b -= len_test + UINT32_C(1);

					if (len_test_b >= UINT32_C(2))
					{
						uint32_t next_rep_match_cost;
						uint32_t state_b = lzma_tab_match_next_states(state);
						uint32_t pos_state_next = (position + len_test) & p->pb_mask;

						uint32_t cur_and_len_byte_cost = cur_and_len_cost + p->prob_costs[p->is_match[state_b][pos_state_next] >> 4] +
							lzma_lit_enc_get_cost_matched((p->lit_probs + ((((position + len_test) & p->lp_mask) << p->lc) + (data[len_test - UINT32_C(1)] >> (UINT32_C(8) - p->lc))) * UINT32_C(0x300)),
								data[len_test], data_b[len_test], p->prob_costs);

						state_b = lzma_tab_literal_next_states(state_b);
						pos_state_next = (pos_state_next + UINT32_C(1)) & p->pb_mask;

						next_rep_match_cost = cur_and_len_byte_cost +
							p->prob_costs[(p->is_match[state_b][pos_state_next] ^ UINT16_C(0x7FF)) >> 4] +
							p->prob_costs[(p->is_rep[state_b] ^ UINT16_C(0x7FF)) >> 4];

						uint32_t offset = cur + len_test + UINT32_C(1) + len_test_b;
						uint32_t cur_and_len_cost_b;
						lzma_optimal_t* opt;

						while (len_end < offset)
							p->opt[++len_end].cost = UINT32_C(0x40000000);

						cur_and_len_cost_b = next_rep_match_cost + lzma_get_rep_cost(p, UINT32_C(0), len_test_b, state_b, pos_state_next);
						opt = &p->opt[offset];

						if (cur_and_len_cost_b < opt->cost)
						{
							opt->cost = cur_and_len_cost_b;
							opt->pos_prev = cur + len_test + UINT32_C(1);
							opt->back_prev = UINT32_C(0);
							opt->prev_a_is_byte = 1;
							opt->prev_b = 1;
							opt->pos_prev_b = cur;
							opt->back_prev_b = cur_back + UINT32_C(4);
						}
					}

					offs += UINT32_C(2);

					if (offs == pair_count)
						break;

					cur_back = matches[offs + UINT32_C(1)];

					if (cur_back >= UINT32_C(0x80))
					{
						uint32_t t = (cur_back < (UINT32_C(1) << ((UINT32_C(9) + sizeof(size_t) / UINT32_C(2)) + UINT32_C(6)))) ? 
							UINT32_C(6) : UINT32_C(6) + (UINT32_C(9) + sizeof(size_t) / UINT32_C(2)) - UINT32_C(1);

						pos_slot = p->fast_pos[cur_back >> t] + (t * UINT32_C(2));
					}
				}
			}
		}
	}
}


inline static uint32_t lzma_get_optimum_fast(lzma_encoder_t* p, uint32_t* back_res)
{
	uint32_t i, available_count, main_len, main_dist, pair_count, rep_index, rep_len;
	const uint8_t* data;
	const uint32_t* matches;

	if (!p->additional_offset)
		main_len = lzma_read_match_dists(p, &pair_count);
	else
	{
		main_len = p->longest_match_length;
		pair_count = p->pair_count;
	}

	available_count = p->available_count;
	*back_res = (uint32_t)-1;

	if (available_count < UINT32_C(2))
		return 1;

	if (available_count > UINT32_C(0x111))
		available_count = UINT32_C(0x111);

	data = lzma_mf_get_ptr_to_current_position(p->match_finder_obj) - UINT32_C(1);

	rep_len = rep_index = UINT32_C(0);

	for (i = UINT32_C(0); i < UINT32_C(4); ++i)
	{
		uint32_t len;
		const uint8_t* data_b = data - p->reps[i] - UINT32_C(1);

		if (data[0] != data_b[0] || data[1] != data_b[1])
			continue;

		for (len = UINT32_C(2); len < available_count && data[len] == data_b[len]; ++len);

		if (len >= p->fast_bytes_count)
		{
			*back_res = i;

			lzma_move_pos(p, len - UINT32_C(1));

			return len;
		}

		if (len > rep_len)
		{
			rep_index = i;
			rep_len = len;
		}
	}

	matches = p->matches;

	if (main_len >= p->fast_bytes_count)
	{
		*back_res = matches[pair_count - UINT32_C(1)] + UINT32_C(4);

		lzma_move_pos(p, main_len - UINT32_C(1));

		return main_len;
	}

	main_dist = UINT32_C(0);

	if (main_len >= UINT32_C(2))
	{
		main_dist = matches[pair_count - UINT32_C(1)];

		while (pair_count > UINT32_C(2) && main_len == matches[pair_count - UINT32_C(4)] + UINT32_C(1))
		{
			if (!((main_dist >> 7) > matches[pair_count - UINT32_C(3)]))
				break;

			pair_count -= UINT32_C(2);
			main_len = matches[pair_count - UINT32_C(2)];
			main_dist = matches[pair_count - UINT32_C(1)];
		}

		if (main_len == UINT32_C(2) && main_dist >= UINT32_C(0x80))
			main_len = UINT32_C(1);
	}

	if (rep_len >= 2 && (rep_len + UINT32_C(1) >= main_len ||
		rep_len + UINT32_C(2) >= main_len && main_dist >= UINT32_C(0x200) ||
		rep_len + UINT32_C(3) >= main_len && main_dist >= UINT32_C(0x8000)))
	{
		*back_res = rep_index;

		lzma_move_pos(p, rep_len - UINT32_C(1));

		return rep_len;
	}

	if (main_len < UINT32_C(2) || available_count <= UINT32_C(2))
		return UINT32_C(1);

	p->longest_match_length = lzma_read_match_dists(p, &p->pair_count);

	if (p->longest_match_length >= UINT32_C(2))
	{
		uint32_t new_dist = matches[p->pair_count - UINT32_C(1)];

		if ((p->longest_match_length >= main_len && new_dist < main_dist) ||
			(p->longest_match_length == main_len + UINT32_C(1) && !((new_dist >> 7) > main_dist)) ||
			p->longest_match_length > main_len + UINT32_C(1) ||
			(p->longest_match_length + UINT32_C(1) >= main_len && main_len >= UINT32_C(3) && ((main_dist >> 7) > new_dist)))
			return UINT32_C(1);
	}

	data = lzma_mf_get_ptr_to_current_position(p->match_finder_obj) - UINT32_C(1);

	for (i = UINT32_C(0); i < UINT32_C(4); ++i)
	{
		uint32_t len, limit;
		const uint8_t *data_b = data - p->reps[i] - UINT32_C(1);

		if (data[0] != data_b[0] || data[1] != data_b[1])
			continue;

		limit = main_len - UINT32_C(1);

		for (len = UINT32_C(2); len < limit && data[len] == data_b[len]; ++len);

		if (len >= limit)
			return UINT32_C(1);
	}

	*back_res = main_dist + UINT32_C(4);

	lzma_move_pos(p, main_len - UINT32_C(2));

	return main_len;
}


inline static void lzma_write_end_marker(lzma_encoder_t* p, uint32_t pos_state)
{
	uint32_t len;

	lzma_range_enc_encode_bit(&p->rc, &p->is_match[p->state][pos_state], UINT32_C(1));
	lzma_range_enc_encode_bit(&p->rc, &p->is_rep[p->state], UINT32_C(0));

	p->state = (p->state < UINT32_C(7)) ? UINT32_C(7) : UINT32_C(10);

	len = UINT32_C(2);

	lzma_len_enc_encode_ex(&p->len_enc, &p->rc, len - UINT32_C(2), pos_state, !p->fast_mode, p->prob_costs);
	lzma_rc_tree_encode(&p->rc, p->pos_slot_encoder[len < UINT32_C(5) ? len - UINT32_C(2) : UINT32_C(3)], UINT32_C(6), UINT32_C(0x3F));
	lzma_range_enc_encode_direct_bits(&p->rc, UINT32_C(0x3FFFFFF), UINT32_C(0x1A));
	lzma_rc_tree_reverse_encode(&p->rc, p->pos_align_encoder, UINT32_C(4), UINT32_C(0xF));
}


inline static lzma_rc_t lzma_check_errors(lzma_encoder_t* p)
{
	if (p->result != LZMA_RC_OK)
		return p->result;

	if (p->rc.res != LZMA_RC_OK)
		p->result = LZMA_RC_WRITE;

	if (p->match_finder_base.result != LZMA_RC_OK)
		p->result = LZMA_RC_READ;

	if (p->result != LZMA_RC_OK)
		p->finished = 1;

	return p->result;
}


inline static lzma_rc_t lzma_flush(lzma_encoder_t* p, uint32_t now_pos)
{
	p->finished = 1;

	if (p->write_end_mark)
		lzma_write_end_marker(p, now_pos & p->pb_mask);

	lzma_range_enc_flush_data(&p->rc);
	lzma_range_enc_flush_stream(&p->rc);

	return lzma_check_errors(p);
}


inline static void lzma_fill_align_costs(lzma_encoder_t* p)
{
	uint32_t i;

	for (i = UINT32_C(0); i < UINT32_C(0x10); ++i)
		p->align_costs[i] = lzma_rc_tree_reverse_get_cost(p->pos_align_encoder, UINT32_C(4), i, p->prob_costs);

	p->align_cost_count = UINT32_C(0);
}


inline static void lzma_fill_distances_costs(lzma_encoder_t* p)
{
	uint32_t temp_costs[0x80];
	uint32_t i, len_to_pos_state;

	for (i = UINT32_C(4); i < UINT32_C(0x80); ++i)
	{
		uint32_t pos_slot = p->fast_pos[i];
		uint32_t footer_bits = (pos_slot >> 1) - UINT32_C(1);
		uint32_t base = (UINT32_C(2) | (pos_slot & UINT32_C(1))) << footer_bits;

		temp_costs[i] = lzma_rc_tree_reverse_get_cost(p->pos_encoders + base - pos_slot - UINT32_C(1), footer_bits, i - base, p->prob_costs);
	}

	for (len_to_pos_state = UINT32_C(0); len_to_pos_state < UINT32_C(4); ++len_to_pos_state)
	{
		uint32_t pos_slot;
		const uint16_t* encoder = p->pos_slot_encoder[len_to_pos_state];
		uint32_t* prob_slot_costs = p->prob_slot_costs[len_to_pos_state];

		for (pos_slot = UINT32_C(0); pos_slot < p->dist_table_size; ++pos_slot)
			prob_slot_costs[pos_slot] = lzma_rc_tree_get_cost(encoder, 6, pos_slot, p->prob_costs);

		for (pos_slot = UINT32_C(14); pos_slot < p->dist_table_size; ++pos_slot)
			prob_slot_costs[pos_slot] += (((pos_slot >> 1) - UINT32_C(5)) << 4);

		uint32_t* distance_costs = p->distance_costs[len_to_pos_state];

		for (i = UINT32_C(0); i < UINT32_C(4); ++i)
			distance_costs[i] = prob_slot_costs[i];

		for (; i < UINT32_C(0x80); ++i)
			distance_costs[i] = prob_slot_costs[p->fast_pos[i]] + temp_costs[i];
	}

	p->match_cost_count = UINT32_C(0);
}


inline static lzma_encoder_t* lzma_encoder_construct(lzma_encoder_t* p)
{
	lzma_encoder_properties_t properties;

	lzma_range_enc_construct(&p->rc);
	lzma_mf_construct(&p->match_finder_base);
	lzma_encoder_properties_init(&properties);
	lzma_encoder_set_properties(p, &properties);
	lzma_encoder_fast_pos_init(p->fast_pos);
	lzma_encoder_init_cost_tables(p->prob_costs);

	p->lit_probs = NULL;
	p->save_state.lit_probs = NULL;

	return p;
}


inline static lzma_encoder_handle_t lzma_encoder_create(lzma_allocator_ptr_t allocator)
{
	lzma_encoder_t* p = allocator->allocate(allocator->state, sizeof(lzma_encoder_t));

	if (!p) return p;

	return lzma_encoder_construct(p);
}


inline static void lzma_encoder_free_lits(lzma_encoder_t* p, lzma_allocator_ptr_t allocator)
{
	allocator->release(allocator->state, p->lit_probs);
	allocator->release(allocator->state, p->save_state.lit_probs);

	p->lit_probs = p->save_state.lit_probs = NULL;
}


inline static void lzma_encoder_destruct(lzma_encoder_t* p, lzma_allocator_ptr_t allocator)
{
	lzma_mf_free(&p->match_finder_base, allocator);
	lzma_encoder_free_lits(p, allocator);
	lzma_range_enc_free(&p->rc, allocator);
}


inline static void lzma_encoder_destroy(lzma_encoder_handle_t p, lzma_allocator_ptr_t allocator)
{
	lzma_encoder_destruct((lzma_encoder_t*)p, allocator);
	allocator->release(allocator->state, p);
}


inline static lzma_rc_t lzma_encoder_code_one_block(lzma_encoder_t* p, uint8_t use_limits, uint32_t max_pack_size, uint32_t max_unpack_size)
{
	lzma_rc_t rc;
	uint32_t now_pos_32, start_pos_32;

	if (p->need_init)
	{
		lzma_mf_init(p->match_finder_obj);
		p->need_init = 0;
	}

	if (p->finished)
		return p->result;

	rc = lzma_check_errors(p);

	if (rc) return rc;

	now_pos_32 = (uint32_t)p->now_pos_64;
	start_pos_32 = now_pos_32;

	if (p->now_pos_64 == UINT32_C(0))
	{
		uint32_t pair_count;
		uint8_t cur_byte;

		if (!lzma_mf_get_num_available_bytes(p->match_finder_obj))
			return lzma_flush(p, now_pos_32);

		lzma_read_match_dists(p, &pair_count);
		lzma_range_enc_encode_bit(&p->rc, &p->is_match[p->state][0], UINT32_C(0));

		p->state = lzma_tab_literal_next_states(p->state);
		cur_byte = *(lzma_mf_get_ptr_to_current_position(p->match_finder_obj) - p->additional_offset);

		lzma_lit_enc_encode(&p->rc, p->lit_probs, cur_byte);

		p->additional_offset--;

		now_pos_32++;
	}

	if (lzma_mf_get_num_available_bytes(p->match_finder_obj))
	{
		for (;;)
		{
			uint32_t pos, len, pos_state;

			if (p->fast_mode)
				len = lzma_get_optimum_fast(p, &pos);
			else len = lzma_get_optimum(p, now_pos_32, &pos);

			pos_state = now_pos_32 & p->pb_mask;

			if (len == UINT32_C(1) && pos == (uint32_t)-1)
			{
				uint8_t cur_byte;
				uint16_t* probabilities;
				const uint8_t* data;

				lzma_range_enc_encode_bit(&p->rc, &p->is_match[p->state][pos_state], UINT32_C(0));

				data = lzma_mf_get_ptr_to_current_position(p->match_finder_obj) - p->additional_offset;

				cur_byte = *data;

				probabilities = p->lit_probs + (((now_pos_32 & p->lp_mask) << p->lc) + ((*(data - UINT32_C(1))) >> (UINT32_C(8) - p->lc))) * UINT32_C(0x300);

				if (p->state < UINT32_C(7))
					lzma_lit_enc_encode(&p->rc, probabilities, cur_byte);
				else lzma_lit_enc_encode_matched(&p->rc, probabilities, cur_byte, *(data - p->reps[0] - UINT32_C(1)));

				p->state = lzma_tab_literal_next_states(p->state);
			}
			else
			{
				lzma_range_enc_encode_bit(&p->rc, &p->is_match[p->state][pos_state], UINT32_C(1));

				if (pos < UINT32_C(4))
				{
					lzma_range_enc_encode_bit(&p->rc, &p->is_rep[p->state], UINT32_C(1));

					if (!pos)
					{
						lzma_range_enc_encode_bit(&p->rc, &p->is_rep_g_0[p->state], UINT32_C(0));
						lzma_range_enc_encode_bit(&p->rc, &p->is_rep_0_long[p->state][pos_state], (len == UINT32_C(1)) ? UINT32_C(0) : UINT32_C(1));
					}
					else
					{
						uint32_t distance = p->reps[pos];

						lzma_range_enc_encode_bit(&p->rc, &p->is_rep_g_0[p->state], 1);

						if (pos == UINT32_C(1))
							lzma_range_enc_encode_bit(&p->rc, &p->is_rep_g_1[p->state], UINT32_C(0));
						else
						{
							lzma_range_enc_encode_bit(&p->rc, &p->is_rep_g_1[p->state], UINT32_C(1));
							lzma_range_enc_encode_bit(&p->rc, &p->is_rep_g_2[p->state], pos - UINT32_C(2));

							if (pos == UINT32_C(3))
								p->reps[3] = p->reps[2];

							p->reps[2] = p->reps[1];
						}

						p->reps[1] = p->reps[0];
						p->reps[0] = distance;
					}

					if (len == UINT32_C(1))
						p->state = lzma_tab_short_replace_next_states(p->state);
					else
					{
						lzma_len_enc_encode_ex(&p->rep_len_enc, &p->rc, len - UINT32_C(2), pos_state, !p->fast_mode, p->prob_costs);
						p->state = lzma_tab_replace_next_states(p->state);
					}
				}
				else
				{
					uint32_t pos_slot;

					lzma_range_enc_encode_bit(&p->rc, &p->is_rep[p->state], UINT32_C(0));

					p->state = lzma_tab_match_next_states(p->state);

					lzma_len_enc_encode_ex(&p->len_enc, &p->rc, len - UINT32_C(2), pos_state, !p->fast_mode, p->prob_costs);

					pos -= UINT32_C(4);

					if (pos < UINT32_C(0x80))
						pos_slot = p->fast_pos[pos];
					else
					{
						uint32_t t = (pos < (UINT32_C(1) << ((UINT32_C(9) + sizeof(size_t) / UINT32_C(2)) + 6))) ?
							UINT32_C(6) : UINT32_C(6) + (UINT32_C(9) + sizeof(size_t) / UINT32_C(2)) - UINT32_C(1);

						pos_slot = p->fast_pos[pos >> t] + (t * UINT32_C(2));
					}

					lzma_rc_tree_encode(&p->rc, p->pos_slot_encoder[(len < UINT32_C(5)) ? len - UINT32_C(2) : UINT32_C(3)], UINT32_C(6), pos_slot);

					if (pos_slot >= UINT32_C(4))
					{
						uint32_t footer_bits = ((pos_slot >> UINT32_C(1)) - UINT32_C(1));
						uint32_t base = ((UINT32_C(2) | (pos_slot & UINT32_C(1))) << footer_bits);
						uint32_t pos_reduced = pos - base;

						if (pos_slot < UINT32_C(14))
							lzma_rc_tree_reverse_encode(&p->rc, p->pos_encoders + base - pos_slot - UINT32_C(1), footer_bits, pos_reduced);
						else
						{
							lzma_range_enc_encode_direct_bits(&p->rc, pos_reduced >> 4, footer_bits - UINT32_C(4));
							lzma_rc_tree_reverse_encode(&p->rc, p->pos_align_encoder, 4, pos_reduced & UINT32_C(0xF));

							p->align_cost_count++;
						}
					}

					p->reps[3] = p->reps[2];
					p->reps[2] = p->reps[1];
					p->reps[1] = p->reps[0];
					p->reps[0] = pos;

					p->match_cost_count++;
				}
			}

			p->additional_offset -= len;
			now_pos_32 += len;

			if (!p->additional_offset)
			{
				uint32_t processed;

				if (!p->fast_mode)
				{
					if (p->match_cost_count >= UINT32_C(0x80))
						lzma_fill_distances_costs(p);

					if (p->align_cost_count >= UINT32_C(0x10))
						lzma_fill_align_costs(p);
				}

				if (!lzma_mf_get_num_available_bytes(p->match_finder_obj))
					break;

				processed = now_pos_32 - start_pos_32;

				if (use_limits)
				{
					if (processed + UINT32_C(0x112C) >= max_unpack_size ||
						(p->rc.processed + (p->rc.buffer - p->rc.buffer_base) + p->rc.cache_size) + UINT32_C(0x1000) * UINT32_C(2) >= max_pack_size)
						break;
				}
				else if (processed >= UINT32_C(0x20000))
				{
					p->now_pos_64 += now_pos_32 - start_pos_32;

					return lzma_check_errors(p);
				}
			}
		}
	}

	p->now_pos_64 += now_pos_32 - start_pos_32;

	return lzma_flush(p, now_pos_32);
}


inline static lzma_rc_t lzma_encoder_allocate(lzma_encoder_t* p, uint32_t keep_window_size, lzma_allocator_ptr_t allocator)
{
	uint32_t before_size = UINT32_C(0x1000);

	if (!lzma_range_enc_allocate(&p->rc, allocator))
		return LZMA_RC_MEMORY;

	uint32_t lclp = p->lc + p->lp;

	if (!p->lit_probs || !p->save_state.lit_probs || p->lclp != lclp)
	{
		lzma_encoder_free_lits(p, allocator);
		p->lit_probs = (uint16_t*)allocator->allocate(allocator->state, (UINT32_C(0x300) << lclp) * sizeof(uint16_t));
		p->save_state.lit_probs = (uint16_t*)allocator->allocate(allocator->state, (UINT32_C(0x300) << lclp) * sizeof(uint16_t));

		if (!p->lit_probs || !p->save_state.lit_probs)
		{
			lzma_encoder_free_lits(p, allocator);

			return LZMA_RC_MEMORY;
		}

		p->lclp = lclp;
	}

	p->match_finder_base.big_hash = (p->dictionary_size > UINT32_C(0x1000000)) ? 1 : 0;

	if (before_size + p->dictionary_size < keep_window_size)
		before_size = keep_window_size - p->dictionary_size;

	if (!lzma_mf_create(&p->match_finder_base, p->dictionary_size, before_size, p->fast_bytes_count, UINT32_C(0x111), allocator))
		return LZMA_RC_MEMORY;

	p->match_finder_obj = &p->match_finder_base;

	return LZMA_RC_OK;
}


void lzma_encoder_init(lzma_encoder_t* p)
{
	uint32_t i;

	p->state = UINT32_C(0);

	for (i = UINT32_C(0); i < UINT32_C(4); ++i)
		p->reps[i] = UINT32_C(0);

	lzma_range_enc_init(&p->rc);

	for (i = UINT32_C(0); i < UINT32_C(12); ++i)
	{
		uint32_t j;

		for (j = UINT32_C(0); j < UINT32_C(0x10); ++j)
		{
			p->is_match[i][j] = UINT16_C(0x400);
			p->is_rep_0_long[i][j] = UINT16_C(0x400);
		}

		p->is_rep[i] = p->is_rep_g_0[i] = p->is_rep_g_1[i] = p->is_rep_g_2[i] = UINT16_C(0x400);
	}

	uint32_t num = UINT32_C(0x300) << (p->lp + p->lc);
	uint16_t* probabilities = p->lit_probs;

	for (i = UINT32_C(0); i < num; ++i)
		probabilities[i] = UINT16_C(0x400);

	for (i = UINT32_C(0); i < UINT32_C(4); ++i)
	{
		uint16_t* probabilities = p->pos_slot_encoder[i];
		uint32_t j;

		for (j = UINT32_C(0); j < UINT32_C(0x40); ++j)
			probabilities[j] = UINT16_C(0x400);
	}

	for (i = UINT32_C(0); i < UINT32_C(0x72); ++i)
		p->pos_encoders[i] = UINT16_C(0x400);

	lzma_len_enc_init(&p->len_enc.p);
	lzma_len_enc_init(&p->rep_len_enc.p);

	for (i = UINT32_C(0); i < UINT32_C(0x10); ++i)
		p->pos_align_encoder[i] = UINT16_C(0x400);

	p->optimum_end_index = p->optimum_current_index = p->additional_offset = UINT32_C(0);

	p->pb_mask = (UINT32_C(1) << p->pb) - UINT32_C(1);
	p->lp_mask = (UINT32_C(1) << p->lp) - UINT32_C(1);
}


void lzma_encoder_init_costs(lzma_encoder_t* p)
{
	if (!p->fast_mode)
	{
		lzma_fill_distances_costs(p);
		lzma_fill_align_costs(p);
	}

	p->len_enc.table_size = p->rep_len_enc.table_size = p->fast_bytes_count - UINT32_C(1);

	lzma_len_cost_enc_update_tabs(&p->len_enc, UINT32_C(1) << p->pb, p->prob_costs);
	lzma_len_cost_enc_update_tabs(&p->rep_len_enc, UINT32_C(1) << p->pb, p->prob_costs);
}


inline static lzma_rc_t lzma_encoder_allocate_and_init(lzma_encoder_t* p, uint32_t keep_window_size, lzma_allocator_ptr_t allocator)
{
	uint32_t i;
	lzma_rc_t rc;

	for (i = UINT32_C(0); i < (((UINT32_C(9) + sizeof(size_t) / UINT32_C(2)) - UINT32_C(1)) * UINT32_C(9)); ++i)
		if (p->dictionary_size <= (UINT32_C(1) << i)) break;

	p->dist_table_size = i * UINT32_C(2);
	p->finished = 0;
	p->result = LZMA_RC_OK;

	rc = lzma_encoder_allocate(p, keep_window_size, allocator); 
	
	if (rc) return rc;

	lzma_encoder_init(p);
	lzma_encoder_init_costs(p);

	p->now_pos_64 = UINT64_C(0);

	return LZMA_RC_OK;
}


inline static lzma_rc_t lzma_encoder_prepare(lzma_encoder_handle_t pp, lzma_seq_out_strm_buf_t* out_stream, lzma_seq_in_strm_buf_t* in_stream, lzma_allocator_ptr_t allocator)
{
	lzma_encoder_t* p = (lzma_encoder_t*)pp;

	p->match_finder_base.stream = in_stream;
	p->need_init = 1;
	p->rc.out_stream = out_stream;

	return lzma_encoder_allocate_and_init(p, UINT32_C(0), allocator);
}


lzma_rc_t lzma_encoder_prepare_for_2(lzma_encoder_handle_t pp, lzma_seq_in_strm_buf_t* in_stream, uint32_t keep_window_size, lzma_allocator_ptr_t allocator)
{
	lzma_encoder_t* p = (lzma_encoder_t*)pp;

	p->match_finder_base.stream = in_stream;
	p->need_init = 1;

	return lzma_encoder_allocate_and_init(p, keep_window_size, allocator);
}


inline static void lzma_encoder_set_input_buffer(lzma_encoder_t* p, const uint8_t* src, size_t src_len)
{
	p->match_finder_base.direct_input = 1;
	p->match_finder_base.buffer_base = (uint8_t*)src;
	p->match_finder_base.direct_input_remaining = src_len;
}


lzma_rc_t lzma_encoder_prepare_memory(lzma_encoder_handle_t pp, const uint8_t* src, size_t src_len, uint32_t keep_window_size, lzma_allocator_ptr_t allocator)
{
	lzma_encoder_t* p = (lzma_encoder_t*)pp;

	lzma_encoder_set_input_buffer(p, src, src_len);

	p->need_init = 1;

	lzma_encoder_set_data_size(pp, src_len);

	return lzma_encoder_allocate_and_init(p, keep_window_size, allocator);
}


inline static size_t lzma_seq_out_strm_buf_write(lzma_seq_out_strm_buf_t* p, const void *data, size_t size)
{
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


inline static uint32_t lzma_encoder_get_num_avail_bytes(lzma_encoder_handle_t pp)
{
	return lzma_mf_get_num_available_bytes(((lzma_encoder_t*)pp)->match_finder_obj);
}


const uint8_t* lzma_encoder_get_cur_buf(lzma_encoder_handle_t pp)
{
	const lzma_encoder_t* p = (lzma_encoder_t*)pp;
	return lzma_mf_get_ptr_to_current_position(p->match_finder_obj) - p->additional_offset;
}


lzma_rc_t lzma_encoder_code_one_mem_block(lzma_encoder_handle_t pp, uint8_t re_init, uint8_t* dest, size_t* dest_len, uint32_t want_pack_size, uint32_t* unpacked_size)
{
	lzma_encoder_t* p = (lzma_encoder_t*)pp;
	uint64_t now_pos_64;
	lzma_rc_t res;
	lzma_seq_out_strm_buf_t out_stream;

	out_stream.data = dest;
	out_stream.rem = *dest_len;
	out_stream.overflow = 0;

	p->write_end_mark = 0;
	p->finished = 0;
	p->result = LZMA_RC_OK;

	if (re_init)
		lzma_encoder_init(p);

	lzma_encoder_init_costs(p);

	now_pos_64 = p->now_pos_64;
	lzma_range_enc_init(&p->rc);
	p->rc.out_stream = &out_stream;

	res = lzma_encoder_code_one_block(p, 1, want_pack_size, *unpacked_size);

	*unpacked_size = (uint32_t)(p->now_pos_64 - now_pos_64);
	*dest_len -= out_stream.rem;

	if (out_stream.overflow)
		return LZMA_RC_OUTPUT_EOF;

	return res;
}


inline static lzma_rc_t lzma_encoder_encode_2(lzma_encoder_t* p)
{
	lzma_rc_t rc = LZMA_RC_OK;
	uint8_t dummy[0x300];

	dummy[0] = dummy[1] = 0;

	for (;;)
	{
		rc = lzma_encoder_code_one_block(p, 0, UINT32_C(0), UINT32_C(0));

		if (rc || p->finished)
			break;
	}

	return rc;
}


lzma_rc_t lzma_encoder_encode(lzma_encoder_handle_t pp, lzma_seq_out_strm_buf_t* out_stream, lzma_seq_in_strm_buf_t* in_stream, lzma_allocator_ptr_t allocator)
{
	lzma_rc_t rc;

	rc = lzma_encoder_prepare(pp, out_stream, in_stream, allocator); 
	
	if (rc) return rc;

	return lzma_encoder_encode_2((lzma_encoder_t*)pp);
}


lzma_rc_t lzma_encoder_write_properties(lzma_encoder_handle_t pp, uint8_t* properties, size_t* size)
{
	lzma_encoder_t* p = (lzma_encoder_t*)pp;
	uint32_t i, dictionary_size = p->dictionary_size;

	if (*size < UINT64_C(5))
		return LZMA_RC_PARAMETER;

	*size = UINT64_C(5);

	properties[0] = (uint8_t)((p->pb * UINT64_C(5) + p->lp) * UINT64_C(9) + p->lc);

	if (dictionary_size >= UINT32_C(0x400000))
	{
		uint32_t dict_mask = UINT32_C(0xFFFFF);

		if (dictionary_size < UINT32_C(0xFFFFFFFF) - dict_mask)
			dictionary_size = (dictionary_size + dict_mask) & ~dict_mask;
	}
	else for (i = UINT32_C(11); i <= UINT32_C(30); ++i)
	{
		if (dictionary_size <= (UINT32_C(2) << i)) { dictionary_size = (UINT32_C(2) << i); break; }
		if (dictionary_size <= (UINT32_C(3) << i)) { dictionary_size = (UINT32_C(3) << i); break; }
	}

	for (i = UINT32_C(0); i < UINT32_C(4); ++i)
		properties[i + UINT32_C(1)] = (uint8_t)(dictionary_size >> (UINT32_C(8) * i));

	return LZMA_RC_OK;
}


uint8_t lzma_encoder_is_write_end_mark(lzma_encoder_handle_t pp)
{
	return ((lzma_encoder_t*)pp)->write_end_mark;
}


lzma_rc_t lzma_encoder_encode_in_memory(lzma_encoder_handle_t pp, uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t src_len, uint8_t write_end_mark, lzma_allocator_ptr_t allocator)
{
	lzma_rc_t res;
	lzma_encoder_t* p = (lzma_encoder_t*)pp;
	lzma_seq_out_strm_buf_t out_stream;

	out_stream.data = dest;
	out_stream.rem = *dest_len;
	out_stream.overflow = 0;

	p->write_end_mark = write_end_mark;
	p->rc.out_stream = &out_stream;

	res = lzma_encoder_prepare_memory(pp, src, src_len, UINT32_C(0), allocator);

	if (res == LZMA_RC_OK)
	{
		res = lzma_encoder_encode_2(p);

		if (res == LZMA_RC_OK && p->now_pos_64 != src_len)
			res = LZMA_RC_FAILURE;
	}

	*dest_len -= out_stream.rem;

	if (out_stream.overflow)
		return LZMA_RC_OUTPUT_EOF;

	return res;
}


exported lzma_rc_t callconv lzma_encode(uint8_t *dest, size_t* dest_len, const uint8_t* src, size_t src_len, const lzma_encoder_properties_t* properties, uint8_t* properties_encoded, size_t* properties_size, uint8_t write_end_mark, lzma_allocator_ptr_t allocator)
{
	lzma_encoder_t* p = (lzma_encoder_t*)lzma_encoder_create(allocator);

	if (!p) return LZMA_RC_MEMORY;

	lzma_rc_t rc = lzma_encoder_set_properties(p, properties);

	if (!rc)
	{
		rc = lzma_encoder_write_properties(p, properties_encoded, properties_size);

		if (!rc)
			rc = lzma_encoder_encode_in_memory(p, dest, dest_len, src, src_len, write_end_mark, allocator);
	}

	lzma_encoder_destroy(p, allocator);

	return rc;
}


exported lzma_rc_t callconv lzma_decode(uint8_t *dest, size_t* dest_len, const uint8_t* src, size_t* src_len, const uint8_t* properties_data, uint32_t properties_size, lzma_finish_mode_t finish_mode, lzma_status_t* status, lzma_allocator_ptr_t allocator)
{
	lzma_rc_t rc;
	lzma_decoder_t p;
	size_t out_size = *dest_len, in_size = *src_len;

	*dest_len = *src_len = UINT64_C(0);
	*status = LZMA_STATUS_NOT_SPECIFIED;

	if (in_size < UINT64_C(5))
		return LZMA_RC_INPUT_EOF;

	p.dictionary = NULL;
	p.probabilities = NULL;

	rc = lzma_decoder_allocate_probabilities(&p, properties_data, properties_size, allocator);

	if (rc) return rc;

	p.dictionary = dest;
	p.dictionary_buffer_size = out_size;

	lzma_decoder_init(&p);

	*src_len = in_size;

	rc = lzma_decoder_decode_to_dictionary(&p, out_size, src, src_len, finish_mode, status);

	*dest_len = p.dictionary_position;

	if (rc == LZMA_RC_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT)
		rc = LZMA_RC_INPUT_EOF;

	lzma_decoder_release_probabilities(&p, allocator);

	return rc;
}


// zmo.c - LZP/PPM compressor adapted from Zhang Li.


#include <stdint.h>

#include "../config.h"


typedef struct ppm_encoder_s
{
	uint32_t low;
	uint32_t range;
	uint32_t size;
	uint32_t index;
	uint8_t* buffer;
}
ppm_encoder_t;


inline static void ppm_encoder_create(register ppm_encoder_t *restrict state, uint8_t* buffer, uint32_t size)
{
	state->low = UINT32_C(0);
	state->range = (uint32_t)-1;
	state->size = size;
	state->index = UINT32_C(0);
	state->buffer = buffer;
}


inline static void ppm_encoder_encode(register ppm_encoder_t *restrict state, uint16_t cumulate, uint16_t frequency, uint16_t sum)
{
	register uint32_t range = state->range / sum;
	register uint32_t low = state->low + (cumulate * range);
	register uint32_t index = state->index;
	register uint8_t* buffer = state->buffer;

	range *= frequency;

	while ((low ^ (low + range)) < UINT32_C(0x1000000) || 
		(range < UINT32_C(0x400000) && ((range = -low & (UINT32_C(0x400000) - UINT32_C(1))), UINT32_C(1))))
	{
		buffer[index] = (uint8_t)(low >> 24);
		index++;
		low <<= 8;
		range <<= 8;
	}

	state->range = range;
	state->low = low;
	state->index = index;
}


inline static ppm_encoder_flush(register ppm_encoder_t *restrict state)
{
	state->buffer[state->index] = (uint8_t)(state->low >> 24), state->index++, state->low <<= 8;
	state->buffer[state->index] = (uint8_t)(state->low >> 24), state->index++, state->low <<= 8;
	state->buffer[state->index] = (uint8_t)(state->low >> 24), state->index++, state->low <<= 8;
	state->buffer[state->index] = (uint8_t)(state->low >> 24), state->index++, state->low <<= 8;
}


typedef struct ppm_decoder_s
{
	uint32_t low;
	uint32_t range;
	uint32_t code;
	uint32_t size;
	uint32_t index;
	uint8_t* buffer;
}
ppm_decoder_t;


inline static void ppm_decoder_create(register ppm_decoder_t *restrict state, uint8_t* buffer, uint32_t size)
{
	state->low = UINT32_C(0);
	state->range = (uint32_t)-1;
	state->code = UINT32_C(0);
	state->size = size;
	state->index = UINT32_C(0);
	state->buffer = buffer;

	state->code = state->code << 8 | state->buffer[state->index], state->index++;
	state->code = state->code << 8 | state->buffer[state->index], state->index++;
	state->code = state->code << 8 | state->buffer[state->index], state->index++;
	state->code = state->code << 8 | state->buffer[state->index], state->index++;
}


inline static void ppm_decoder_decode(register ppm_decoder_t *restrict state, uint16_t cumulate, uint16_t frequency)
{
	register uint32_t low = state->low + (cumulate * state->range);
	register uint32_t range = state->range * frequency;
	register uint32_t code = state->code;
	register uint32_t index = state->index;
	register uint8_t* buffer = state->buffer;

	while ((low ^ (low + range)) < UINT32_C(0x1000000) || (range < UINT32_C(0x400000) && ((range = -low & (UINT32_C(0x400000) - UINT32_C(1))), UINT32_C(1))))
	{
		code = code << 8 | buffer[index];
		range <<= 8;
		low <<= 8;
	}

	state->low = low;
	state->range = range;
	state->code = code;
	state->index = index;
}

inline static uint16_t ppm_decoder_decode_cumulate(register ppm_decoder_t *restrict state, uint16_t sum)
{
	state->range /= sum;
	return (state->code - state->low) / state->range;
}


typedef struct ppm_symbol_counter_s
{
    uint8_t symbol;
    uint8_t frequency;
}
ppm_symbol_counter_t;


inline static void ppm_symbol_counter_create(register ppm_symbol_counter_t *restrict state)
{
	state->symbol = UINT8_C(0);
	state->frequency = UINT8_C(0);
}


typedef struct ppm_bit_model_s
{
	uint16_t c[2];
}
ppm_bit_model_t;


inline static void ppm_bit_model_create(register ppm_bit_model_t *restrict state)
{
	state->c[0] = state->c[1] = UINT16_C(0);
}


inline static uint8_t ppm_bit_model_encode(register ppm_bit_model_t *restrict state, register ppm_encoder_t *restrict coder, uint8_t b)
{
	if (b == UINT8_C(0)) 
		ppm_encoder_encode(coder, 0, state->c[0], state->c[0] + state->c[1]);
	else ppm_encoder_encode(coder, state->c[0], state->c[1], state->c[0] + state->c[1]);

	return b;
}


inline static uint8_t ppm_bit_model_decode(register ppm_bit_model_t *restrict state, register ppm_decoder_t *restrict coder)
{
	if (state->c[0] > ppm_decoder_decode_cumulate(coder, state->c[0] + state->c[1]))
	{
		ppm_decoder_decode(coder, UINT16_C(0), state->c[0]);
		return UINT8_C(0);
	}
	else 
	{
		ppm_decoder_decode(coder, state->c[0], state->c[1]);
		return UINT8_C(1);
	}
}


inline static void ppm_bit_model_update(register ppm_bit_model_t *restrict state, uint8_t b) 
{
	if ((state->c[b] += UINT16_C(15)) > UINT16_C(9000))
	{
		state->c[0] = (uint16_t)((state->c[0] + UINT16_C(1)) * 0.9);
		state->c[1] = (uint16_t)((state->c[1] + UINT16_C(1)) * 0.9);
	}
}


typedef struct ppm_dense_model_s
{
	uint16_t sum;
	uint16_t count;
	uint16_t escape;
	ppm_symbol_counter_t symbols[256];
}
ppm_dense_model_t;


inline static void ppm_dense_model_create(register ppm_dense_model_t *restrict state)
{
	state->sum = UINT16_C(0);
	state->count = UINT16_C(0);
	state->escape = UINT16_C(0);
	register uint32_t i = UINT32_C(0);
	for (; i < UINT32_C(256); ++i)
		ppm_symbol_counter_create(&state->symbols[i]);
}


typedef uint8_t ppm_bitset_t[256];


inline static uint8_t ppm_bitset_any(register ppm_bitset_t v)
{
	register uint16_t i = UINT16_C(0);
	for (; i < UINT16_C(256); ++i)
		if (v[i]) return UINT8_C(1);
	return UINT8_C(0);
}


inline static uint8_t ppm_dense_model_encode(register ppm_dense_model_t *restrict state, ppm_encoder_t *restrict coder, ppm_bitset_t exclude, uint8_t b)
{
	register uint32_t i, n;
	uint32_t position = UINT32_C(0);
	uint16_t cumulate = UINT16_C(0), sum = UINT16_C(0), escape = UINT16_C(0), frequency = UINT16_C(0);
	uint16_t recent = state->symbols[0].frequency & -!exclude[state->symbols[0].symbol];
	uint8_t found = UINT8_C(0);

	if (!ppm_bitset_any(exclude))
	{
		n = (uint32_t)state->count;

		for (i = UINT32_C(0); i < n; ++i)
		{
			if (state->symbols[i].symbol == b)
			{
				position = i;
				found = UINT8_C(1);
				break;
			}

			cumulate += state->symbols[i].frequency;
		}

		sum = state->sum;
	}
	else 
	{
		n = (uint32_t)state->count;

		for (i = UINT32_C(0); i < n; ++i) 
		{
			if (state->symbols[i].symbol == b) 
			{
				position = i;
				found = UINT8_C(1);
			}

			cumulate += state->symbols[i].frequency & -(!exclude[state->symbols[i].symbol] && !found);
			sum += state->symbols[i].frequency & -(!exclude[state->symbols[i].symbol]);
		}
	}

	escape = state->escape + !state->escape;
	sum += recent + escape;
	frequency = state->symbols[position].frequency;

	if (position == UINT32_C(0))
		frequency += recent;
	else 
	{
		uint8_t swpf = state->symbols[position].frequency;
		uint8_t swps = state->symbols[position].symbol;

		state->symbols[position].frequency = state->symbols[0].frequency;
		state->symbols[position].symbol = state->symbols[0].symbol;

		state->symbols[0].frequency = swpf;
		state->symbols[0].symbol = swps;

		cumulate += recent;
	}

	if (!found) 
	{
		n = (uint32_t)state->count;

		for (i = UINT32_C(0); i < n; ++i)
			exclude[state->symbols[i].symbol] = UINT8_C(1);
		
		state->symbols[n].frequency = state->symbols[0].frequency;
		state->symbols[n].symbol = state->symbols[0].symbol;

		state->symbols[0].frequency = UINT8_C(0);
		state->symbols[0].symbol = b;
		
		state->count++;
		cumulate = sum - escape;
		frequency = escape;
	}

	ppm_encoder_encode(coder, cumulate, frequency, sum);

	return found;
}


inline static uint16_t ppm_dense_model_decode(register ppm_dense_model_t *restrict state, ppm_decoder_t *restrict coder, ppm_bitset_t exclude)
{
	register uint32_t i, n;
	uint16_t cumulate = UINT16_C(0), frequency = UINT16_C(0), sum = UINT16_C(0), escape = UINT16_C(0);
	uint8_t recent = state->symbols[0].frequency & -!exclude[state->symbols[0].symbol];
	int16_t symbol = INT16_C(-1);

	n = state->count;

	for (i = UINT32_C(0); i < n; ++i)
		sum += state->symbols[i].frequency & -!exclude[state->symbols[i].symbol];

	escape = state->escape + !state->escape;
	sum += recent + escape;

	uint16_t decode_cumulate = ppm_decoder_decode_cumulate(coder, sum);

	if (sum - escape <= decode_cumulate) 
	{
		n = state->count;

		for (i = UINT32_C(0); i < n; ++i)
			exclude[state->symbols[i].symbol] = UINT8_C(1);
		
		state->symbols[state->count].frequency = state->symbols[0].frequency;
		state->symbols[state->count].symbol = state->symbols[0].symbol;
		state->symbols[0].frequency = UINT8_C(0);
		state->count++;

		cumulate = sum - escape;
		frequency = escape;
	}
	else 
	{
		i = UINT32_C(0);

		if (!ppm_bitset_any(exclude))
		{
			while (cumulate + recent + state->symbols[i].frequency <= decode_cumulate)
			{
				cumulate += state->symbols[i].frequency;
				i++;
			}
		}
		else 
		{
			while (cumulate + recent + (state->symbols[i].frequency & -!exclude[state->symbols[i].symbol]) <= decode_cumulate) 
			{
				cumulate += state->symbols[i].frequency & -!exclude[state->symbols[i].symbol];
				i++;
			}
		}

		frequency = state->symbols[i].frequency;
		symbol = state->symbols[i].symbol;

		if (i == UINT32_C(0)) frequency += recent;
		else 
		{
			uint8_t swpf = state->symbols[i].frequency;
			uint8_t swps = state->symbols[i].symbol;

			state->symbols[i].frequency = state->symbols[0].frequency;
			state->symbols[i].symbol = state->symbols[0].symbol;

			state->symbols[0].frequency = swpf;
			state->symbols[0].symbol = swps;

			cumulate += recent;
		}
	}

	ppm_decoder_decode(coder, cumulate, frequency);

	return symbol;
}


inline static void ppm_dense_model_update(register ppm_dense_model_t *restrict state, uint8_t b)
{
	register uint32_t i, n;

	state->symbols[0].frequency++;
	state->symbols[0].symbol = b;
	state->sum++;
	state->escape += (state->symbols[0].frequency == UINT8_C(1)) - (state->symbols[0].frequency == UINT8_C(2));

	if (state->symbols[0].frequency <= UINT8_C(250)) return;

	state->count = state->sum = state->escape = UINT16_C(0);

	n = UINT32_C(0);

	for (i = UINT32_C(0); i + n < UINT32_C(256); ++i)
	{
		if ((state->symbols[i].frequency = state->symbols[i + n].frequency / UINT8_C(2)) > UINT8_C(0))
		{
			state->symbols[i].symbol = state->symbols[i + n].symbol;
			state->count++;
			state->sum += state->symbols[i].frequency;
			state->escape += (state->symbols[i].frequency == UINT8_C(1));
		}
		else
		{
			n++;
			i--;
		}
	}

	for (i = (uint32_t)state->count; i + n < UINT32_C(256); ++i)
	{
		state->symbols[i].frequency = UINT8_C(0);
		state->symbols[i].symbol = UINT8_C(0);
	}
}


typedef struct ppm_sparse_model_s
{
	struct ppm_sparse_model_s* next;
	uint16_t sum;
	uint8_t count;
	uint8_t visited;
	uint64_t context : 48;
	ppm_symbol_counter_t symbols[54];
}
ppm_sparse_model_t;


inline static void ppm_sparse_model_create(register ppm_sparse_model_t *restrict state)
{
	state->next = NULL;
	state->sum = UINT16_C(0);
	state->count = UINT8_C(0);
	state->visited = UINT8_C(0);
	state->context = UINT64_C(0);

	register uint32_t i = UINT32_C(0);

	for (; i < UINT8_C(54); ++i)
		ppm_symbol_counter_create(&state->symbols[i]);
}


inline static ppm_symbol_counter_t* ppm_symbol_counter_copy(ppm_symbol_counter_t* first, ppm_symbol_counter_t* last, ppm_symbol_counter_t* result)
{
	while (first != last) 
	{
		result->frequency = first->frequency;
		result->symbol = first->symbol;
		++result; 
		++first;
	}

	return result;
}


inline static void ppm_symbol_counter_fill(ppm_symbol_counter_t* first, ppm_symbol_counter_t* last, ppm_symbol_counter_t* value)
{
	while (first != last) 
	{
		first->symbol = value->symbol;
		first->frequency = value->frequency;
		++first;
	}
}


inline static uint8_t ppm_sparse_model_encode(register ppm_sparse_model_t *restrict state, ppm_bit_model_t *restrict model, ppm_encoder_t *restrict coder, uint8_t b, ppm_bitset_t exclude)
{
	register uint32_t i, n;
	uint16_t cumulate = UINT16_C(0), frequency = UINT16_C(0);
	int32_t found = INT32_C(-1);

	n = state->count;

	for (i = UINT32_C(0); i < n; ++i)
	{
		if (state->symbols[i].symbol == b)
		{
			found = (int32_t)i;
			break;
		}

		cumulate += state->symbols[i].frequency;
	}

	if (found >= INT32_C(0))
	{
		ppm_bit_model_encode(model, coder, 0);
		ppm_bit_model_update(model, 0);

		if (state->count != UINT8_C(1))
		{
			uint16_t recent = (((uint16_t)state->symbols[0].frequency) + UINT16_C(6)) / UINT16_C(2);

			if (found == INT32_C(0))
				frequency = ((uint16_t)state->symbols[found].frequency) + recent;
			else
			{
				frequency = (uint16_t)state->symbols[found].frequency;
				cumulate += recent;

				ppm_symbol_counter_t temp = { state->symbols[found].symbol, state->symbols[found].frequency };

				ppm_symbol_counter_copy(&state->symbols[0], &state->symbols[found], &state->symbols[1]);

				state->symbols[0].symbol = temp.symbol;
				state->symbols[0].frequency = temp.frequency;
			}

			ppm_encoder_encode(coder, cumulate, frequency, state->sum + recent);
		}

		return UINT8_C(1);
	}

	ppm_bit_model_encode(model, coder, UINT8_C(1));
	ppm_bit_model_update(model, UINT8_C(1));

	n = state->count;

	for (i = UINT32_C(0); i < n; ++i)
		exclude[state->symbols[i].symbol] = UINT8_C(1);

	if (state->count == 54)
		state->sum -= (uint16_t)state->symbols[state->count - UINT8_C(1)].frequency;
	else state->count++;

	ppm_symbol_counter_copy(&state->symbols[0], &state->symbols[state->count - UINT8_C(1)], &state->symbols[1]);

	state->symbols[0].symbol = b;
	state->symbols[0].frequency = UINT8_C(0);

	return UINT8_C(0);
}


inline static int16_t ppm_sparse_model_decode(register ppm_sparse_model_t *restrict state, ppm_bit_model_t *restrict model, ppm_decoder_t *restrict coder, ppm_bitset_t exclude)
{
	register uint32_t i, n;
	uint16_t cumulate = UINT16_C(0), frequency = UINT16_C(0);

	if (ppm_bit_model_decode(model, coder) == UINT8_C(0)) 
	{
		ppm_bit_model_update(model, UINT8_C(0));

		if (state->count != UINT8_C(1))
		{
			uint16_t recent = (((uint16_t)state->symbols[0].frequency) + UINT16_C(6)) / UINT16_C(2);
			uint16_t decode = ppm_decoder_decode_cumulate(coder, state->sum + recent);

			i = UINT32_C(0);

			while (cumulate + recent + state->symbols[i].frequency <= decode)
			{
				cumulate += state->symbols[i].frequency;
				i++;
			}

			if (i == UINT32_C(0))
				frequency = state->symbols[i].frequency + recent;
			else 
			{
				frequency = state->symbols[i].frequency;
				cumulate += recent;

				ppm_symbol_counter_t temp = { state->symbols[i].symbol, state->symbols[i].frequency };

				ppm_symbol_counter_copy(&state->symbols[0], &state->symbols[i], &state->symbols[1]);

				state->symbols[0].symbol = temp.symbol;
				state->symbols[0].frequency = temp.frequency;
			}

			ppm_decoder_decode(coder, cumulate, frequency);
		}

		return (int16_t)state->symbols[0].symbol;

	}
	else
	{
		ppm_bit_model_update(model, UINT8_C(1));

		n = (uint32_t)state->count;

		for (i = UINT32_C(0); i < n; ++i)
			exclude[state->symbols[i].symbol] = UINT8_C(1);
		
		if (state->count == 54)
			state->sum -= state->symbols[state->count - UINT8_C(1)].frequency;
		else state->count++;
		
		ppm_symbol_counter_copy(&state->symbols[0], &state->symbols[state->count - UINT8_C(1)], &state->symbols[1]);

		state->symbols[0].frequency = UINT8_C(0);
	}

	return INT16_C(-1);
}


inline static void ppm_sparse_model_update(register ppm_sparse_model_t *restrict state, ppm_dense_model_t *restrict lower, uint8_t b)
{
	register uint32_t i, n;

	if (state->symbols[0].frequency == 0)
	{
		ppm_symbol_counter_t counter;
		ppm_symbol_counter_create(&counter);

		n = (uint32_t)lower->count;

		for (i = UINT32_C(0); i < n; ++i) 
		{
			if (lower->symbols[i].symbol == b) 
			{
				counter.symbol = lower->symbols[i].symbol;
				counter.frequency = lower->symbols[i].frequency;
				break;
			}
		}

		state->symbols[0].frequency = (uint8_t)(UINT16_C(2) + (((uint16_t)counter.frequency) * UINT16_C(16) > lower->sum));
		state->symbols[0].symbol = b;
		state->sum += (uint16_t)state->symbols[0].frequency;
	}
	else 
	{
		uint8_t increment = UINT8_C(1) + (state->symbols[0].frequency <= UINT8_C(3)) + (state->symbols[0].frequency <= UINT8_C(220));
		state->symbols[0].symbol = b;
		state->symbols[0].frequency += increment;
		state->sum += (uint16_t)increment;
	}

	if (state->symbols[0].frequency > 250)
	{
		n = UINT32_C(0);
		state->count = UINT8_C(0);
		state->sum = UINT16_C(0);

		for (i = UINT32_C(0); i + n < UINT32_C(54); ++i)
		{
			if ((state->symbols[i].frequency = state->symbols[i + n].frequency / UINT8_C(2)) > 0)
			{
				state->symbols[i].symbol = state->symbols[i + n].symbol;
				state->count++;
				state->sum += (uint16_t)state->symbols[i].frequency;
			}
			else 
			{
				n++;
				i--;
			}
		}

		ppm_symbol_counter_t blank;
		ppm_symbol_counter_create(&blank);
		ppm_symbol_counter_fill(&state->symbols[state->count], &state->symbols[53], &blank);
	}
}


typedef struct ppm_model_s
{
	ppm_bit_model_t see[131072];
	ppm_sparse_model_t* o4_buckets[262144];
	ppm_dense_model_t o2[65536];
	ppm_dense_model_t o1[256];
	ppm_dense_model_t o0[1];
	uint32_t o4_count;
	uint64_t context;
	uint8_t see_ch_context;
	uint8_t see_last_esc;
}
ppm_model_t;


inline static void ppm_model_create(register ppm_model_t *restrict state)
{
	uint32_t i;

	state->o4_count = UINT32_C(0);
	state->context = UINT64_C(0);
	state->see_ch_context = UINT8_C(0);
	state->see_last_esc = UINT8_C(0);

	for (i = UINT32_C(0); i < UINT32_C(131072); ++i)
	{
		state->see[i].c[0] = UINT16_C(20);
		state->see[i].c[1] = UINT16_C(10);
	}
}


inline static uint32_t ppm_log_2(register uint32_t value)
{
	register uint32_t result = (value > UINT32_C(0xFFFF)) << 4; value >>= result;
	register uint32_t shift = (value > UINT32_C(0xFF)) << 3; value >>= shift; result |= shift;
	shift = (value > UINT32_C(0xF)) << 2; value >>= shift; result |= shift;
	shift = (value > UINT32_C(0x3)) << 1; value >>= shift; result |= shift;
	return result | (value >> 1);
}


#define ppm_min(p, q) (((p) < (q)) ? (p) : (q))
#define ppm_max(p, q) (((p) > (q)) ? (p) : (q))


inline static ppm_dense_model_t* ppm_model_current_o2(register ppm_model_t *restrict state) { return &state->o2[state->context & UINT64_C(0xFFFF)]; }
inline static ppm_dense_model_t* ppm_model_current_o1(register ppm_model_t *restrict state) { return &state->o1[state->context & UINT64_C(0x00FF)]; }
inline static ppm_dense_model_t* ppm_model_current_o0(register ppm_model_t *restrict state) { return &state->o0[0]; }


inline static ppm_bit_model_t* ppm_model_current_see(register ppm_model_t *restrict state, ppm_sparse_model_t *restrict o4)
{
	static ppm_bit_model_t zero = { { 0, 1 } };

	if (o4->count == UINT8_C(0)) return &zero;

	uint8_t current = o4->count;
	ppm_dense_model_t* o2 = ppm_model_current_o2(state);
	uint16_t sum = o2->sum;
	uint8_t low = o2->count;
	uint64_t context = 0 | ((state->context >> 6) & 3) << 0 | ((state->context >> 14) & 3) << 2 | ((state->context >> 22) & 3) << 4 | 
		state->see_last_esc << 6;

	if (current == 1) 
	{
		context |= 0
			| (o4->symbols[0].symbol >> 5) << 7
			| (sum >= 5) << 10
			| ppm_min(ppm_log_2(current / 2), 3) << 11
			| ppm_min(ppm_log_2(o4->sum / 3), 7) << 13
			| 1 << 16;
		return &state->see[context];
	}

	context |= 0 | ppm_min(ppm_log_2(ppm_max(low - current, 0) / 2), 3) << 7 | (sum >= 5) << 10 | 
		ppm_min(ppm_log_2(current / 2), 3) << 11 | ppm_min(ppm_log_2(o4->sum / 8), 7) << 13 | 0 << 16;

	return &state->see[context];
}


ppm_sparse_model_t* ppm_model_current_o4(register ppm_model_t *restrict state)
{
	if (state->o4_count >= UINT32_C(262144) * 5)
	{
		for (auto bucket : state->o4_buckets)
		{
			auto it0 = bucket;
			auto it1 = bucket ? bucket->next : NULL;

			while (it1)
			{
				if ((it1->visited /= 2) == 0)
				{
					it0->next = it1->next;
					delete it1;
					state->o4_count--;
					it1 = it0->next;

					continue;
				}

				it0 = it1;
				it1 = it1->next;
			}
		}
	}

	uint64_t compacted_context = 0 | (state->context & 0xc0ffffffffff);
	auto& bucket = state->o4_buckets[((compacted_context >> 16) * 13131 + compacted_context) % UINT32_C(262144)];
	auto it0 = bucket;
	auto it1 = bucket;

	while (it1 != NULL) 
	{
		if (it1->m_context == compacted_context)
		{
			if (it1 != bucket)
			{
				it0->next = it1->next;
				it1->next = bucket;
				bucket = it1;
			}

			it1->visited += (it1->visited < 255);

			return it1;
		}

		it0 = it1;
		it1 = it1->next;
	}

	auto new_node = new sparse_model_t();

	new_node->context = compacted_context;
	new_node->visited = 1;
	new_node->next = bucket;
	bucket = new_node;

	state->o4_count++;

	return new_node;
}


inline static void ppm_model_encode(register ppm_model_t *restrict state, ppm_encoder_t *restrict coder, int c)
{
	ppm_sparse_model_t* o4 = ppm_model_current_o4(state);
	ppm_dense_model_t* o2 = ppm_model_current_o2(state);
	ppm_dense_model_t* o1 = ppm_model_current_o1(state);
	ppm_dense_model_t* o0 = ppm_model_current_o0(state);

	auto order = 0;
	ppm_bitset_t exclude;

	while (-1)
	{
		order = 4; if (ppm_sparse_model_encode(o4, ppm_model_current_see(state, o4), coder, c, exclude)) break;
		order = 2; if (ppm_dense_model_encode(o2, coder, exclude, c)) break;
		order = 1; if (ppm_dense_model_encode(o1, coder, exclude, c)) break;
		order = 0; if (ppm_dense_model_encode(o0, coder, exclude, c)) break;

		auto cumulate = 0;

		for (auto i = 0; i < c; i++)
			cumulate += !exclude[i];
		
		ppm_encoder_encode(coder, cumulate, 1, 256 - exclude.count());

		break;
	}

	switch (order)
	{
	case 0: o0->update(c);
	case 1: o1->update(c);
	case 2: o2->update(c);
	case 4: o4->update(o2, c);
	}

	m_see_last_esc = (order == 4);
}



struct ppm_model_t
{

    

    // main ppm-decode method
    int decode(rc_decoder_t* coder) {
        auto o4 = current_o4();
        auto o2 = current_o2();
        auto o1 = current_o1();
        auto o0 = current_o0();
        auto order = 0;
        auto c = 0;
        auto exclude = std::bitset<256>();

        while (-1) {
            order = 4; if ((c = o4->decode(current_see(o4), coder, exclude)) != -1) break;
            order = 2; if ((c = o2->decode(coder, exclude)) != -1) break;
            order = 1; if ((c = o1->decode(coder, exclude)) != -1) break;
            order = 0; if ((c = o0->decode(coder, exclude)) != -1) break;

            // decode with o(-1)
            auto decode_cum = coder->decode_cum(256 - exclude.count());
            auto cumulate = 0;
            for (c = 0; cumulate + !exclude[c] <= decode_cum; c++) {
                cumulate += !exclude[c];
            }
            coder->decode(cumulate, 1);
            break;
        }
        switch (order) {  // fall-through switch
            case 0: o0->update(c);
            case 1: o1->update(c);
            case 2: o2->update(c);
            case 4: o4->update(o2, c);
        }
        m_see_last_esc = (order == 4);
        return c;
    }

    void update_context(int c) {
        m_context = m_context << 8 | c;
    }
};

/*******************************************************************************
 * Matcher
 ******************************************************************************/
struct matcher_t {
    static const auto match_min = 12;
    static const auto match_max = 255;
    std::array<uint64_t, 1048576> m_lzp;  // lzp = pos[32] + checksum[16] + prefetch[16]

    matcher_t() {
        m_lzp.fill(0);
    }

    static uint32_t hash2(unsigned char* p) {
        return uint32_t(p[1] * 1919191 + p[0]) % 1048576;
    }
    static uint32_t hash5(unsigned char* p) {
        return uint32_t(p[0] * 1717171 + p[1] * 17171 + p[2] * 171 + p[3]) % 1048576;
    }
    static uint32_t hash8(unsigned char* p) {
        return uint32_t(
                p[0] * 13131313 + p[1] * 1313131 + p[2] * 131313 + p[3] * 13131 +
                p[4] * 1313     + p[5] * 131     + p[6] * 13     + p[7] * 1) % 1048576;
    }

    uint64_t getlzp(unsigned char* data, uint32_t pos) {
        if (pos >= 8) {
            auto lzp8 = m_lzp[hash8(data + pos - 8)];
            auto lzp5 = m_lzp[hash5(data + pos - 5)];
            auto lzp2 = m_lzp[hash2(data + pos - 2)];
            if ((lzp8 >> 32 & 0xffff) == *(uint16_t*)(data + pos - 2) && (lzp8 & 0xffffffff) != 0) return lzp8;
            if ((lzp5 >> 32 & 0xffff) == *(uint16_t*)(data + pos - 2) && (lzp5 & 0xffffffff) != 0) return lzp5;
            if ((lzp2 >> 32 & 0xffff) == *(uint16_t*)(data + pos - 2) && (lzp2 & 0xffffffff) != 0) return lzp2;
        }
        return 0;
    }

    uint32_t getpos(unsigned char* data, uint32_t pos) {
        return getlzp(data, pos) & 0xffffffff;
    }

    uint32_t lookup(unsigned char* data, uint32_t data_size, uint32_t pos, int do_lazy_match = 1, int maxlen = match_max) {
        auto match_lzp = getlzp(data, pos);
        if ((match_lzp >> 48 & 0xffff) != *(uint16_t*)(data + pos + match_min - 2)) {
            return 1;
        }
        auto match_pos = match_lzp & 0xffffffff;
        auto match_len = 0;
        if (match_pos > 0) {
            while (match_pos + match_len < data_size
                    && match_len < maxlen
                    && data[match_pos + match_len] == data[pos + match_len]) {
                match_len++;
            }
        }
        if (do_lazy_match) {
            auto next_match_len = lookup(data, data_size, pos + 1, 0, match_len + 2);
            if (match_len + 1 < next_match_len) {
                return 1;
            }
        }
        return (match_len >= match_min) ? match_len : 1;
    }

    void update(unsigned char* data, uint32_t pos) {
        if (pos >= 8) {  // avoid overflow
            (m_lzp[hash8(data + pos - 8)] =
             m_lzp[hash5(data + pos - 5)] =
             m_lzp[hash2(data + pos - 2)] = (0
                 | (uint64_t) pos
                 | (uint64_t) *(uint16_t*) (data + pos - 2) << 32
                 | (uint64_t) *(uint16_t*) (data + pos + match_min - 2) << 48));
        }
    }
};

/*******************************************************************************
 * Codec
 ******************************************************************************/
static const auto BLOCK_SIZE = 16777216;
static const auto MATCH_LENS_SIZE = 64000;

void zmolly_encode(std::istream& orig, std::ostream& comp) {
    auto ppm = std::make_unique<ppm_model_t>();
    auto orig_data = std::make_unique<unsigned char[]>(BLOCK_SIZE);

    while (orig.peek() != EOF) {
        orig.read((char*) &orig_data[0], BLOCK_SIZE);
        auto orig_size = orig.gcount();

        // find escape char
        auto counts = std::array<int, 256>();
        auto escape = 0;
        for (auto i = 0; i < orig_size; i++) {
            counts[orig_data[i]]++;
        }
        for (auto i = 0; i < 256; i++) {
            escape = counts[escape] < counts[i] ? escape : i;
        }

        auto comp_start_pos = comp.tellp();
        auto matcher = std::make_unique<matcher_t>();
        comp.put(escape);

        auto coder = rc_encoder_t(comp);
        auto orig_pos = size_t(0);

        auto match_idx = 0;
        auto match_pos = 0;
        auto thread = std::thread();
        auto match_lens1 = std::array<int, MATCH_LENS_SIZE>();
        auto match_lens2 = std::array<int, MATCH_LENS_SIZE>();
        auto match_lens_current = &match_lens1;
        auto func_matching_thread = [&](auto match_lens) {
            auto match_idx = 0;
            while (std::streampos(match_pos) < orig_size && match_idx < MATCH_LENS_SIZE) {
                auto match_len = matcher->lookup(&orig_data[0], orig_size, match_pos);
                for (auto i = 0; i < match_len; i++) {
                    matcher->update(&orig_data[0], match_pos + i);
                }
                match_pos += match_len;
                match_lens[match_idx++] = match_len;
            }
        };

        // start thread (matching first block)
        thread = std::thread(func_matching_thread, &match_lens1[0]); thread.join();
        thread = std::thread(func_matching_thread, &match_lens2[0]);

        while (orig_pos < orig_size) {
            // find match in separated thread
            if (match_idx >= MATCH_LENS_SIZE) {  // start the next matching thread
                thread.join();
                thread = std::thread(func_matching_thread, &match_lens_current->operator[](0));
                match_lens_current = (*match_lens_current == match_lens1) ? &match_lens2 : &match_lens1;
                match_idx = 0;
            }
            auto match_len = match_lens_current->operator[](match_idx++);

            if (match_len > 1) {  // encode a match
                ppm->encode(&coder, escape);
                ppm->update_context(escape);
                ppm->encode(&coder, match_len);
                ppm->update_context(match_len);
                for (auto i = 0; i < match_len; i++) {
                    ppm->update_context(orig_data[orig_pos++]);
                }

            } else {  // encode a literal
                ppm->encode(&coder, orig_data[orig_pos]);
                ppm->update_context(orig_data[orig_pos]);
                if (orig_data[orig_pos] == escape) {
                    ppm->encode(&coder, 0);
                    ppm->update_context(0);
                }
                orig_pos++;
            }
        }
        thread.join();
        ppm->encode(&coder, escape);  // write end of block code
        ppm->update_context(escape);
        ppm->encode(&coder, orig.peek() != EOF ? 1 : 2);  // 1: end of block, 2: end of input
        coder.flush();
        fprintf(stderr, "encode-block: %zu => %zu\n", orig_pos, size_t(comp.tellp() - comp_start_pos));
    }
}

void zmolly_decode(std::istream& comp, std::ostream& orig) {
    auto ppm = std::make_unique<ppm_model_t>();
    auto end_of_input = false;
    auto orig_data = std::make_unique<unsigned char[]>(BLOCK_SIZE + 1024);

    while (!end_of_input) {
        auto end_of_block = false;
        auto comp_start_pos = comp.tellg();
        auto matcher = std::make_unique<matcher_t>();
        auto escape = comp.get();
        auto coder = rc_decoder_t(comp);
        auto orig_pos = size_t(0);

        while (!end_of_block) {
            auto c = ppm->decode(&coder);
            ppm->update_context(c);
            if (c != escape) {  // literal
                orig_data[orig_pos] = c;
                matcher->update(&orig_data[0], orig_pos);
                orig_pos++;
            } else {
                auto match_len = ppm->decode(&coder);
                if (match_len >= matcher_t::match_min && match_len <= matcher_t::match_max) {  // match
                    auto match_pos = matcher->getpos(&orig_data[0], orig_pos);
                    for (auto i = 0; i < match_len; i++) {  // update context
                        orig_data[orig_pos] = orig_data[match_pos];
                        ppm->update_context(orig_data[orig_pos]);
                        matcher->update(&orig_data[0], orig_pos);
                        orig_pos++;
                        match_pos++;
                    }
                } else if (match_len == 0) {  // escape literal
                    orig_data[orig_pos] = escape;
                    ppm->update_context(orig_data[orig_pos]);
                    matcher->update(&orig_data[0], orig_pos);
                    orig_pos++;
                } else if (match_len == 1) {  // end of block
                    end_of_block = true;
                } else if (match_len == 2) {  // end of block
                    end_of_block = true;
                    end_of_input = true;
                } else {
                    throw std::runtime_error("invalid input data");
                }
            }
            if (orig_pos > BLOCK_SIZE) {
                throw std::runtime_error("invalid input data");
            }
        }
        orig.write((char*) &orig_data[0], orig_pos);
        fprintf(stderr, "decode-block: %zu <= %zu\n", orig_pos, size_t(comp.tellg() - comp_start_pos));
    }
}

/*******************************************************************************
 * Main
 ******************************************************************************/
int main(int argc, char** argv) {
    fprintf(stderr,
            "zmolly:\n"
            "  simple LZP/PPM data compressor.\n"
            "  author: Zhang Li <richselian@gmail.com>\n"
            "usage:\n"
            "  encode: zmolly e inputFile outputFile\n"
            "  decode: zmolly d inputFile outputFile\n");

    // check args
    if (argc != 4) {
        throw std::runtime_error(std::string() + "invalid number of arguments");
    }
    if (std::string() + argv[1] != "e" && std::string() + argv[1] != std::string("d")) {
        throw std::runtime_error(std::string() + "error: invalid mode: " + argv[1]);
    }

    // open input file
    auto fin = std::ifstream(std::string() + argv[2], std::ios::in | std::ios::binary);
    fin.exceptions(std::ios_base::failbit);
    if (!fin.is_open()) {
        throw std::runtime_error(std::string() + "cannot open input file: " + argv[2]);
    }

    // open output file
    auto fout = std::ofstream(argv[3], std::ios::out | std::ios::binary);
    fin.exceptions(std::ios_base::failbit);
    if (!fout.is_open()) {
        throw std::runtime_error(std::string() + "cannot open output file: " + argv[3]);
    }

    // encode/decode
    if (std::string() + argv[1] == "e") zmolly_encode(fin, fout);
    if (std::string() + argv[1] == "d") zmolly_decode(fin, fout);
    return 0;
}

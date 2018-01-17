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


inline static void* ppm_memset(void *restrict p, register uint8_t v, register size_t n)
{
	register uint8_t* d = (uint8_t*)p;
	while (n-- > 0U) *d++ = v;
	return p;
}


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
	state->buffer[state->index] = (uint8_t)(state->low >> 24); state->index++; state->low <<= 8;
	state->buffer[state->index] = (uint8_t)(state->low >> 24); state->index++; state->low <<= 8;
	state->buffer[state->index] = (uint8_t)(state->low >> 24); state->index++; state->low <<= 8;
	state->buffer[state->index] = (uint8_t)(state->low >> 24); state->index++; state->low <<= 8;
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

	state->code = state->code << 8 | state->buffer[state->index]; state->index++;
	state->code = state->code << 8 | state->buffer[state->index]; state->index++;
	state->code = state->code << 8 | state->buffer[state->index]; state->index++;
	state->code = state->code << 8 | state->buffer[state->index]; state->index++;
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


#define PPM_DENSE_MODEL_SYMBOLS_SIZE 0x100U


typedef struct ppm_dense_model_s
{
	uint16_t sum;
	uint16_t count;
	uint16_t escape;
	ppm_symbol_counter_t symbols[PPM_DENSE_MODEL_SYMBOLS_SIZE];
}
ppm_dense_model_t;


inline static void ppm_dense_model_create(register ppm_dense_model_t *restrict state)
{
	state->sum = UINT16_C(0);
	state->count = UINT16_C(0);
	state->escape = UINT16_C(0);

	register uint32_t i = UINT32_C(0);

	for (; i < PPM_DENSE_MODEL_SYMBOLS_SIZE; ++i)
		ppm_symbol_counter_create(&state->symbols[i]);
}


typedef struct ppm_bitset_256_s { uint64_t q[4]; } ppm_bitset_256_t;

//typedef uint8_t ppm_bitset_256_t[0x100];


inline static uint8_t ppm_bitset_256_any(register ppm_bitset_256_t *restrict s)
{
	return (uint8_t)(s->q[0] || s->q[1] || s->q[2] || s->q[3]);
}


inline static uint8_t ppm_bitset_256_get(register ppm_bitset_256_t *restrict s, uint8_t index)
{
	uint64_t f = (index) ? (index / UINT64_C(64)) : UINT64_C(0);
	uint64_t i = (f) ? index - (f * UINT64_C(64)) : index;
	return (uint8_t)!!((s->q[f] >> i) & UINT64_C(1));
}


inline static void ppm_bitset_256_set(register ppm_bitset_256_t *restrict s, uint8_t index, uint8_t value)
{
	uint64_t f = (index) ? (index / UINT64_C(64)) : UINT64_C(0);
	uint64_t i = (f) ? index - (f * UINT64_C(64)) : index;
	s->q[f] ^= (-((uint64_t)!!value) ^ s->q[f]) & (UINT64_C(1) << i);
}


inline static uint16_t ppm_bitset_256_count(ppm_bitset_256_t *restrict s)
{
	register uint16_t c = 0;
	register uint64_t t;

	t = s->q[0];
	t = t - ((t >> 1) & ~UINT64_C(0) / UINT64_C(3));
	t = (t & ~UINT64_C(0) / UINT64_C(0xF) * UINT64_C(3)) + ((t >> 2) & ~UINT64_C(0) / UINT64_C(0xF) * UINT64_C(3));
	t = (t + (t >> 4)) & ~UINT64_C(0) / UINT64_C(0xFF) * UINT64_C(0xF);
	c += (uint16_t)((t * (~UINT64_C(0) / UINT64_C(0xFF))) >> 7 * CHAR_BIT);

	t = s->q[1];
	t = t - ((t >> 1) & ~UINT64_C(0) / UINT64_C(3));
	t = (t & ~UINT64_C(0) / UINT64_C(0xF) * UINT64_C(3)) + ((t >> 2) & ~UINT64_C(0) / UINT64_C(0xF) * UINT64_C(3));
	t = (t + (t >> 4)) & ~UINT64_C(0) / UINT64_C(0xFF) * UINT64_C(0xF);
	c += (uint16_t)((t * (~UINT64_C(0) / UINT64_C(0xFF))) >> 7 * CHAR_BIT);

	t = s->q[2];
	t = t - ((t >> 1) & ~UINT64_C(0) / UINT64_C(3));
	t = (t & ~UINT64_C(0) / UINT64_C(0xF) * UINT64_C(3)) + ((t >> 2) & ~UINT64_C(0) / UINT64_C(0xF) * UINT64_C(3));
	t = (t + (t >> 4)) & ~UINT64_C(0) / UINT64_C(0xFF) * UINT64_C(0xF);
	c += (uint16_t)((t * (~UINT64_C(0) / UINT64_C(0xFF))) >> 7 * CHAR_BIT);

	t = s->q[3];
	t = t - ((t >> 1) & ~UINT64_C(0) / UINT64_C(3));
	t = (t & ~UINT64_C(0) / UINT64_C(0xF) * UINT64_C(3)) + ((t >> 2) & ~UINT64_C(0) / UINT64_C(0xF) * UINT64_C(3));
	t = (t + (t >> 4)) & ~UINT64_C(0) / UINT64_C(0xFF) * UINT64_C(0xF);
	c += (uint16_t)((t * (~UINT64_C(0) / UINT64_C(0xFF))) >> 7 * CHAR_BIT);

	return c;
}


inline static uint8_t ppm_bitset_256_clear(register ppm_bitset_256_t *restrict s)
{
	s->q[0] = s->q[1] = s->q[2] = s->q[3] = UINT64_C(0);
}


inline static uint8_t ppm_dense_model_encode(register ppm_dense_model_t *restrict state, ppm_encoder_t *restrict coder, ppm_bitset_256_t *restrict exclude, uint8_t b)
{
	register uint32_t i, n;
	uint32_t position = UINT32_C(0);
	uint16_t cumulate = UINT16_C(0), sum = UINT16_C(0), escape = UINT16_C(0), frequency = UINT16_C(0);
	uint16_t recent = state->symbols[0].frequency & -!ppm_bitset_256_get(exclude, state->symbols[0].symbol);
	uint8_t found = UINT8_C(0);

	if (!ppm_bitset_256_any(exclude))
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

			cumulate += state->symbols[i].frequency & -(!ppm_bitset_256_get(exclude, state->symbols[i].symbol) && !found);
			sum += state->symbols[i].frequency & -(!ppm_bitset_256_get(exclude, state->symbols[i].symbol));
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
			ppm_bitset_256_set(exclude, state->symbols[i].symbol, 1);
		
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


inline static int16_t ppm_dense_model_decode(register ppm_dense_model_t *restrict state, ppm_decoder_t *restrict coder, ppm_bitset_256_t* exclude)
{
	register uint32_t i, n;
	uint16_t cumulate = UINT16_C(0), frequency = UINT16_C(0), sum = UINT16_C(0), escape = UINT16_C(0);
	uint8_t recent = state->symbols[0].frequency & -!ppm_bitset_256_get(exclude, state->symbols[0].symbol);
	int16_t symbol = INT16_C(-1);

	n = state->count;

	for (i = UINT32_C(0); i < n; ++i)
		sum += state->symbols[i].frequency & -!ppm_bitset_256_get(exclude, state->symbols[i].symbol);

	escape = state->escape + !state->escape;
	sum += recent + escape;

	uint16_t decode_cumulate = ppm_decoder_decode_cumulate(coder, sum);

	if (sum - escape <= decode_cumulate) 
	{
		n = state->count;

		for (i = UINT32_C(0); i < n; ++i)
			ppm_bitset_256_set(exclude, state->symbols[i].symbol, 1);
		
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

		if (!ppm_bitset_256_any(exclude))
		{
			while (cumulate + recent + state->symbols[i].frequency <= decode_cumulate)
			{
				cumulate += state->symbols[i].frequency;
				i++;
			}
		}
		else 
		{
			while (cumulate + recent + (state->symbols[i].frequency & -!ppm_bitset_256_get(exclude, state->symbols[i].symbol)) <= decode_cumulate)
			{
				cumulate += state->symbols[i].frequency & -!ppm_bitset_256_get(exclude, state->symbols[i].symbol);
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

	if (state->symbols[0].frequency <= UINT8_C(0xFA)) return;

	state->count = state->sum = state->escape = UINT16_C(0);

	n = UINT32_C(0);

	for (i = UINT32_C(0); i + n < UINT32_C(0x100); ++i)
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

	for (i = (uint32_t)state->count; i + n < UINT32_C(0x100); ++i)
	{
		state->symbols[i].frequency = UINT8_C(0);
		state->symbols[i].symbol = UINT8_C(0);
	}
}


#define PPM_SPARSE_MODEL_SYMBOLS_SIZE 54


typedef struct ppm_sparse_model_s
{
	struct ppm_sparse_model_s* next;
	uint16_t sum;
	uint8_t count;
	uint8_t visited;
	uint64_t context : 48;
	ppm_symbol_counter_t symbols[PPM_SPARSE_MODEL_SYMBOLS_SIZE];
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

	for (; i < PPM_SPARSE_MODEL_SYMBOLS_SIZE; ++i)
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


inline static uint8_t ppm_sparse_model_encode(register ppm_sparse_model_t *restrict state, ppm_bit_model_t *restrict model, ppm_encoder_t *restrict coder, uint8_t b, ppm_bitset_256_t *restrict exclude)
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
		ppm_bitset_256_set(exclude, state->symbols[i].symbol, 1);
	
	if (state->count == PPM_SPARSE_MODEL_SYMBOLS_SIZE)
		state->sum -= (uint16_t)state->symbols[state->count - UINT8_C(1)].frequency;
	else state->count++;

	ppm_symbol_counter_copy(&state->symbols[0], &state->symbols[state->count - UINT8_C(1)], &state->symbols[1]);

	state->symbols[0].symbol = b;
	state->symbols[0].frequency = UINT8_C(0);

	return UINT8_C(0);
}


inline static int16_t ppm_sparse_model_decode(register ppm_sparse_model_t *restrict state, ppm_bit_model_t *restrict model, ppm_decoder_t *restrict coder, ppm_bitset_256_t *restrict exclude)
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
			ppm_bitset_256_set(exclude, state->symbols[i].symbol, 1);
		
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
		
		for (i = UINT32_C(0); i + n < PPM_SPARSE_MODEL_SYMBOLS_SIZE; ++i)
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
		ppm_symbol_counter_fill(&state->symbols[state->count], &state->symbols[PPM_SPARSE_MODEL_SYMBOLS_SIZE - 1], &blank);
	}
}


#define PPM_MODEL_SEE_SIZE 0x20000U
#define PPM_MODEL_O4_SIZE 0x40000U
#define PPM_MODEL_O2_SIZE 0x10000U
#define PPM_MODEL_O1_SIZE 0x100U


typedef struct ppm_model_s
{
	ppm_bit_model_t see[PPM_MODEL_SEE_SIZE];
	ppm_sparse_model_t* o4_buckets[PPM_MODEL_O4_SIZE];
	ppm_dense_model_t o2[PPM_MODEL_O2_SIZE];
	ppm_dense_model_t o1[PPM_MODEL_O1_SIZE];
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

	for (i = UINT32_C(0); i < PPM_MODEL_SEE_SIZE; ++i)
	{
		state->see[i].c[0] = UINT16_C(20);
		state->see[i].c[1] = UINT16_C(10);
	}

	for (i = UINT32_C(0); i < PPM_MODEL_O4_SIZE; ++i)
		state->o4_buckets[i] = NULL;
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
	uint64_t context = UINT64_C(0) | ((state->context >> 6) & UINT64_C(3)) << 0 | ((state->context >> 14) & UINT64_C(3)) << 2 | 
		((state->context >> 22) & UINT64_C(3)) << 4 | state->see_last_esc << 6;

	if (current == UINT8_C(1)) 
	{
		context |= UINT64_C(0) | (uint64_t)(o4->symbols[0].symbol >> 5) << 7 | (uint64_t)(sum >= UINT16_C(5)) << 10 | 
			(uint64_t)ppm_min(ppm_log_2((uint32_t)current / UINT32_C(2)), UINT32_C(3)) << 11 | 
			(uint64_t)ppm_min(ppm_log_2(o4->sum / UINT32_C(3)), UINT32_C(7)) << 13 | UINT64_C(1) << 16;

		return &state->see[context];
	}

	context |= UINT64_C(0) | (uint64_t)ppm_min(ppm_log_2(ppm_max(low - current, 0) / UINT64_C(2)), UINT64_C(3)) << 7 | 
		(uint64_t)(sum >= UINT16_C(5)) << 10 | (uint64_t)ppm_min(ppm_log_2((uint32_t)(current / UINT32_C(2))), UINT64_C(3)) << 11 |
		(uint64_t)ppm_min(ppm_log_2((uint32_t)o4->sum / UINT32_C(8)), UINT64_C(7)) << 13 | UINT64_C(0) << 16;

	return &state->see[context];
}


ppm_sparse_model_t* ppm_model_current_o4(register ppm_model_t *restrict state)
{
	register uint32_t i, n;

	if (state->o4_count >= PPM_MODEL_O4_SIZE * 5U)
	{
		n = PPM_MODEL_O4_SIZE;

		for (i = UINT32_C(0); i < n; ++i)
		{
			ppm_sparse_model_t* bucket = state->o4_buckets[i];
			ppm_sparse_model_t* it0 = bucket;
			ppm_sparse_model_t* it1 = bucket ? bucket->next : NULL;

			while (it1)
			{
				if ((it1->visited /= UINT8_C(2)) == UINT8_C(0))
				{
					it0->next = it1->next;

					free(it1);

					state->o4_count--;
					it1 = it0->next;

					continue;
				}

				it0 = it1;
				it1 = it1->next;
			}
		}
	}

	uint64_t compacted_context = UINT64_C(0) | (state->context & UINT64_C(0xC0FFFFFFFFFF));
	ppm_sparse_model_t* bucket = state->o4_buckets[((compacted_context >> 16) * UINT64_C(0x334B) + compacted_context) % PPM_MODEL_O4_SIZE];
	ppm_sparse_model_t* it0 = bucket;
	ppm_sparse_model_t* it1 = bucket;

	while (it1 != NULL) 
	{
		if (it1->context == compacted_context)
		{
			if (it1 != bucket)
			{
				it0->next = it1->next;
				it1->next = bucket;
				bucket = it1;
			}

			it1->visited += (it1->visited < UINT8_C(0xFF));

			return it1;
		}

		it0 = it1;
		it1 = it1->next;
	}

	ppm_sparse_model_t* node = (ppm_sparse_model_t*)malloc(sizeof(ppm_sparse_model_t));

	node->context = compacted_context;
	node->visited = UINT8_C(1);
	node->next = bucket;

	bucket = node;

	state->o4_count++;

	return node;
}


inline static void ppm_model_encode(register ppm_model_t *restrict state, ppm_encoder_t *restrict coder, uint8_t b)
{
	register uint32_t i, n;

	ppm_sparse_model_t* o4 = ppm_model_current_o4(state);
	ppm_dense_model_t* o2 = ppm_model_current_o2(state);
	ppm_dense_model_t* o1 = ppm_model_current_o1(state);
	ppm_dense_model_t* o0 = ppm_model_current_o0(state);

	uint8_t order = UINT8_C(0);
	ppm_bitset_256_t exclude;

	ppm_bitset_256_clear(&exclude);

	while (-1)
	{
		order = UINT8_C(4); if (ppm_sparse_model_encode(o4, ppm_model_current_see(state, o4), coder, b, &exclude)) break;
		order = UINT8_C(2); if (ppm_dense_model_encode(o2, coder, &exclude, b)) break;
		order = UINT8_C(1); if (ppm_dense_model_encode(o1, coder, &exclude, b)) break;
		order = UINT8_C(0); if (ppm_dense_model_encode(o0, coder, &exclude, b)) break;

		uint16_t cumulate = UINT16_C(0);

		n = (uint32_t)b;

		for (i = UINT32_C(0); i < n; ++i)
			cumulate += !ppm_bitset_256_get(&exclude, i);
		
		ppm_encoder_encode(coder, cumulate, UINT16_C(1), UINT16_C(0x100) - ppm_bitset_256_count(&exclude));

		break;
	}

	if (order == UINT8_C(0)) { ppm_dense_model_update(o0, b); goto LOC_1; }
	else if (order == UINT8_C(1)) { LOC_1: ppm_dense_model_update(o1, b); goto LOC_2; }
	else if (order == UINT8_C(2)) { LOC_2: ppm_dense_model_update(o2, b); goto LOC_4; }
	else if (order == UINT8_C(4)) { LOC_4: ppm_sparse_model_update(o4, o2, b); }

	state->see_last_esc = (order == UINT8_C(4));
}


inline static int ppm_model_decode(register ppm_model_t *restrict state, ppm_decoder_t *restrict coder)
{
	ppm_sparse_model_t* o4 = ppm_model_current_o4(state);
	ppm_dense_model_t* o2 = ppm_model_current_o2(state);
	ppm_dense_model_t* o1 = ppm_model_current_o1(state);
	ppm_dense_model_t* o0 = ppm_model_current_o0(state);

	uint8_t order = UINT8_C(0);
	int16_t c = UINT16_C(0);

	ppm_bitset_256_t exclude;
	ppm_bitset_256_clear(&exclude);

	while (-1)
	{
		order = UINT8_C(4); if ((c = ppm_sparse_model_decode(o4, ppm_model_current_see(state, o4), coder, &exclude)) != INT16_C(-1)) break;
		order = UINT8_C(2); if ((c = ppm_dense_model_decode(o2, coder, &exclude)) != -1) break;
		order = UINT8_C(1); if ((c = ppm_dense_model_decode(o1, coder, &exclude)) != -1) break;
		order = UINT8_C(0); if ((c = ppm_dense_model_decode(o0, coder, &exclude)) != -1) break;

		uint16_t decode = ppm_decoder_decode_cumulate(coder, UINT16_C(0x100) - ppm_bitset_256_count(&exclude));
		uint16_t cumulate = UINT16_C(0);

		for (c = INT16_C(0); cumulate + !ppm_bitset_256_get(&exclude, c) <= decode; ++c)
			cumulate += !ppm_bitset_256_get(&exclude, c);
		
		ppm_decoder_decode(coder, cumulate, UINT16_C(1));

		break;
	}

	if (order == 0) { ppm_dense_model_update(o0, (uint8_t)c); goto LOC_1; }
	else if (order == 1) { LOC_1: ppm_dense_model_update(o1, (uint8_t)c); goto LOC_2; }
	else if (order == 2) { LOC_2: ppm_dense_model_update(o2, (uint8_t)c); goto LOC_4; }
	else if (order == 4) { LOC_4: ppm_sparse_model_update(o4, o2, (uint8_t)c); }

	state->see_last_esc = (order == UINT8_C(4));

	return c;
}


inline static void ppm_model_update_context(register ppm_model_t *restrict state, int c)
{
	state->context = state->context << 8 | c;
}


#define PPM_MATCHER_MATCH_MIN 0xC
#define PPM_MATCHER_MATCH_MAX 0xFF
#define PPM_MATCHER_LZP_SIZE 0x100000U


typedef struct ppm_matcher_s
{
	uint64_t lzp[PPM_MATCHER_LZP_SIZE];
}
ppm_matcher_t;


inline static void ppm_matcher_create(register ppm_matcher_t *restrict state)
{
	register uint32_t i, n = PPM_MATCHER_LZP_SIZE;

	for (i = UINT32_C(0); i < n; ++i)
		state->lzp[i] = UINT64_C(0);
}


inline static uint32_t ppm_matcher_hash_2(register uint8_t *restrict p)
{
	return (uint32_t)((p[1] * UINT32_C(1919191) + p[0]) % PPM_MATCHER_LZP_SIZE);
}


inline static uint32_t ppm_matcher_hash_5(register uint8_t *restrict p)
{
	return (uint32_t)((p[0] * UINT32_C(1717171) + p[1] * UINT32_C(17171) + p[2] * UINT32_C(171) + p[3]) % PPM_MATCHER_LZP_SIZE);
}


inline static uint32_t ppm_matcher_hash_8(register uint8_t *restrict p)
{
	return (uint32_t)((p[0] * UINT32_C(13131313) + p[1] * UINT32_C(1313131) + p[2] * UINT32_C(131313) + p[3] * UINT32_C(13131) + 
		p[4] * UINT32_C(1313) + p[5] * UINT32_C(131) + p[6] * UINT32_C(13) + p[7] * UINT32_C(1)) % PPM_MATCHER_LZP_SIZE);
}


inline static uint64_t ppm_matcher_get_lzp(register ppm_matcher_t *restrict state, uint8_t* data, uint32_t pos)
{
	if (pos >= UINT32_C(8)) 
	{
		uint64_t lzp8 = state->lzp[ppm_matcher_hash_8(data + pos - 8)];
		uint64_t lzp5 = state->lzp[ppm_matcher_hash_5(data + pos - 5)];
		uint64_t lzp2 = state->lzp[ppm_matcher_hash_2(data + pos - 2)];

		if ((lzp8 >> 32 & UINT64_C(0xFFFF)) == *(uint16_t*)(data + pos - 2) && (lzp8 & UINT64_C(0xFFFFFFFF)) != 0) return lzp8;
		if ((lzp5 >> 32 & UINT64_C(0xFFFF)) == *(uint16_t*)(data + pos - 2) && (lzp5 & UINT64_C(0xFFFFFFFF)) != 0) return lzp5;
		if ((lzp2 >> 32 & UINT64_C(0xFFFF)) == *(uint16_t*)(data + pos - 2) && (lzp2 & UINT64_C(0xFFFFFFFF)) != 0) return lzp2;
	}

	return UINT64_C(0);
}


inline static uint32_t ppm_matcher_get_pos(register ppm_matcher_t *restrict state, uint8_t* data, uint32_t pos)
{
	return (uint32_t)(ppm_matcher_get_lzp(state, data, pos) & UINT64_C(0xFFFFFFFF));
}


inline static uint32_t ppm_matcher_lookup(register ppm_matcher_t *restrict state, uint8_t* data, uint32_t size, uint32_t pos, uint8_t lazy /*= 1*/, uint32_t maximum /*= PPM_MATCHER_MATCH_MAX*/)
{
	uint64_t match = ppm_matcher_get_lzp(state, data, pos);

	if ((match >> 48 & UINT64_C(0xFFFF)) != *(uint16_t*)(data + pos + PPM_MATCHER_MATCH_MIN - UINT32_C(2)))
		return UINT32_C(1);
	
	uint64_t position = match & UINT64_C(0xFFFFFFFF);
	uint64_t length = UINT64_C(0);

	if (position > UINT64_C(0))
		while (position + length < size && length < maximum && data[position + length] == data[pos + length])
			length++;

	if (lazy) 
	{
		uint32_t next = ppm_matcher_lookup(state, data, size, pos + 1, UINT8_C(0), (uint32_t)length + UINT32_C(2));
		if (length + 1 < next)
			return UINT32_C(1);
	}

	return (length >= PPM_MATCHER_MATCH_MIN) ? (uint32_t)length : UINT32_C(1);
}


inline static void ppm_matcher_update(register ppm_matcher_t *restrict state, uint8_t* data, uint32_t pos)
{
	if (pos >= 8) 
	{
		state->lzp[ppm_matcher_hash_8(data + pos - 8)] =
		state->lzp[ppm_matcher_hash_5(data + pos - 5)] =
		state->lzp[ppm_matcher_hash_2(data + pos - 2)] = 
			(UINT64_C(0) | (uint64_t)pos | (uint64_t) *(uint16_t*)(data + pos - 2) << 32 | (uint64_t)*(uint16_t*)(data + pos + PPM_MATCHER_MATCH_MIN - 2) << 48);
	}
}


#define PPM_BLOCK_SIZE 0x1000000U
#define PPM_MATCH_WINDOW_SIZE 0xFA00U


typedef struct ppm_encoder_state_s
{
	ppm_model_t model;
	ppm_matcher_t matcher;
	ppm_encoder_t encoder;
	uint8_t* input;
	size_t input_size;
	size_t input_index;
	uint8_t* output;
	size_t output_size;
	size_t output_index;
	size_t output_start_position;
	uint8_t data[PPM_BLOCK_SIZE];
	size_t data_size;
	uint32_t counts[0x100];
	uint8_t escape;
	uint64_t original_position;
	uint64_t original_size;
	uint64_t match_index;
	uint64_t match_position;
	int32_t match_window_a[PPM_MATCH_WINDOW_SIZE];
	int32_t match_window_b[PPM_MATCH_WINDOW_SIZE];
	int32_t* match_window_current;

}
ppm_encoder_state_t;


inline static void ppm_encode_matching(ppm_encoder_state_t* state, int32_t* window)
{
	register uint32_t i;
	uint32_t index = 0;

	while (state->match_position < state->data_size && index < PPM_MATCH_WINDOW_SIZE)
	{
		uint32_t length = ppm_matcher_lookup(&state->matcher, &state->data[0], state->data_size, state->match_position, 1, PPM_MATCHER_MATCH_MAX);

		for (i = 0; i < length; ++i)
			ppm_matcher_update(&state->matcher, &state->data[0], (uint32_t)state->match_position + i);

		state->match_position += length;
		window[state->match_index++] = length;
	}
}


inline static void* ppm_memcpy(void *restrict p, const void *restrict q, size_t n)
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


exported uint8_t* callconv ppm_encode(uint8_t* input, size_t input_size, uint8_t* output, size_t output_size)
{
	register uint32_t i, n;

	ppm_encoder_state_t* state = (ppm_encoder_state_t*)malloc(sizeof(ppm_encoder_state_t));

	ppm_memset(state, 0, sizeof(ppm_encoder_state_t));

	state->input = input;
	state->input_size = input_size;
	state->input = output;
	state->input_size = output_size;

    while (state->input_index < state->input_size)
	{
		n = state->input_size - state->input_index;
		n = ppm_min(n, PPM_BLOCK_SIZE);

		ppm_memcpy(&state->data[0], &state->input[state->input_index], n);

        state->data_size = state->original_size = n;
		state->input_index += n;

		ppm_memset(&state->counts[0], 0, sizeof(state->counts));
        
		state->escape = UINT8_C(0);

		n = state->original_size;

        for (i = UINT32_C(0); i < n; ++i)
			state->counts[state->data[i]]++;

        for (i = UINT32_C(0); i < UINT32_C(0x100); ++i)
			state->escape = (state->counts[state->escape] < state->counts[i]) ? state->escape : (uint8_t)i;

		state->output_start_position = state->output_index;

		ppm_matcher_create(&state->matcher);
		
		state->output[state->output_index] = state->escape;
		state->output_index++;

		ppm_encoder_create(&state->encoder, state->output, state->output_size);

		state->original_position = UINT64_C(0);
		state->match_index = 0;
		state->match_position = 0;

		ppm_memset(&state->match_window_a[0], 0, sizeof(state->match_window_a));
		ppm_memset(&state->match_window_b[0], 0, sizeof(state->match_window_b));
		
		state->match_window_current = &state->match_window_a[0];

		ppm_encode_matching(state, &state->match_window_a[0]);
		ppm_encode_matching(state, &state->match_window_b[0]);

        while (state->original_position < state->original_size)
		{
            if (state->match_index >= PPM_MATCH_WINDOW_SIZE)
			{
				ppm_encode_matching(state, state->match_window_current);
				state->match_window_current = (state->match_window_current == &state->match_window_a[0]) ? &state->match_window_b[0] : &state->match_window_a[0];
				state->match_index = UINT64_C(0);
            }

            uint32_t match_length = state->match_window_current[state->match_index++];

            if (match_length > UINT32_C(1)) 
			{
				ppm_model_encode(&state->model, &state->encoder, state->escape);
				ppm_model_update_context(&state->model, state->escape);
				ppm_model_encode(&state->model, &state->encoder, match_length);
				ppm_model_update_context(&state->model, match_length);

                for (i = UINT32_C(0); i < match_length; ++i)
					ppm_model_update_context(&state->model, state->data[state->original_position++]);
            } 
			else 
			{
				ppm_model_encode(&state->model, &state->encoder, state->data[state->original_position]);
				ppm_model_update_context(&state->model, state->data[state->original_position]);

                if (state->data[state->original_position] == state->escape)
				{
					ppm_model_encode(&state->model, &state->encoder, UINT8_C(0));
					ppm_model_update_context(&state->model, 0);
                }

				state->original_position++;
            }
        }

		ppm_model_encode(&state->model, &state->encoder, state->escape);
		ppm_model_update_context(&state->model, state->escape);
		ppm_model_encode(&state->model, &state->encoder, (state->input_index < state->input_size) ? 1 : 2);
        ppm_encoder_flush(&state->encoder);
    }
}


exported uint8_t* callconv ppm_decode(uint8_t* input, size_t input_size, uint8_t* output, size_t output_size)
{
    auto ppm = std::make_unique<ppm_model_t>();
    auto end_of_input = false;
    auto orig_data = std::make_unique<unsigned char[]>(PPM_BLOCK_SIZE + 1024);

    while (!end_of_input)
	{
        auto end_of_block = false;
        auto output_start_position = comp.tellg();
        auto matcher = std::make_unique<matcher_t>();
        auto escape = comp.get();
        auto coder = rc_decoder_t(comp);
        auto original_position = size_t(0);

        while (!end_of_block)
		{
            auto c = ppm->decode(&coder);
            ppm->update_context(c);

            if (c != escape) 
			{
                orig_data[original_position] = c;
                matcher->update(&orig_data[0], original_position);
                original_position++;
            } 
			else
			{
                auto match_length = ppm->decode(&coder);

                if (match_length >= PPM_MATCHER_MATCH_MIN && match_length <= PPM_MATCHER_MATCH_MAX) 
				{
                    auto match_position = matcher->ppm_matcher_get_pos(&orig_data[0], original_position);

                    for (auto i = 0; i < match_length; i++)
					{
                        orig_data[original_position] = orig_data[match_position];
                        ppm->update_context(orig_data[original_position]);
                        matcher->update(&orig_data[0], original_position);
                        original_position++;
                        match_position++;
                    }
                } 
				else if (match_length == 0)
				{
                    orig_data[original_position] = escape;
                    ppm->update_context(orig_data[original_position]);
                    matcher->update(&orig_data[0], original_position);
                    original_position++;
                } else if (match_length == 1) {  // end of block
                    end_of_block = true;
                } else if (match_length == 2) {  // end of block
                    end_of_block = true;
                    end_of_input = true;
                } else {
                    throw std::runtime_error("invalid input data");
                }
            }
            if (original_position > PPM_BLOCK_SIZE) {
                throw std::runtime_error("invalid input data");
            }
        }
        orig.write((char*) &orig_data[0], original_position);
        fprintf(stderr, "decode-block: %zu <= %zu\n", original_position, size_t(comp.tellg() - output_start_position));
    }
}



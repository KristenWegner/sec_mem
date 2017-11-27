// hashing.h - Hashing


#include <stdint.h>

#include "config.h"


#ifndef INCLUDE_HASHING_H
#define INCLUDE_HASHING_H 1


// Definitions


#define sec_hashing_murmur3_seed_32 (UINT32_C(0x1F36DDD3))                 
#define sec_hashing_murmur3_seed_64_a (UINT64_C(0x005C7F7D1D097179))
#define sec_hashing_murmur3_seed_64_b (UINT64_C(0x0000245A37832C20))


#define sec_hashing_swap_64(x) ((uint64_t)( \
	(((uint64_t)(x) & UINT64_C(0x00000000000000FF)) << 56) | \
	(((uint64_t)(x) & UINT64_C(0x000000000000FF00)) << 40) | \
	(((uint64_t)(x) & UINT64_C(0x0000000000FF0000)) << 24) | \
	(((uint64_t)(x) & UINT64_C(0x00000000FF000000)) << 8)  | \
	(((uint64_t)(x) & UINT64_C(0x000000FF00000000)) >> 8)  | \
	(((uint64_t)(x) & UINT64_C(0x0000FF0000000000)) >> 24) | \
	(((uint64_t)(x) & UINT64_C(0x00FF000000000000)) >> 40) | \
	(((uint64_t)(x) & UINT64_C(0xFF00000000000000)) >> 56)))


#ifdef SEC_WORDS_BIG_ENDIAN
#define sec_hashing_swap_order_64(x) (sec_hashing_swap_64(x))
#else
#define sec_hashing_swap_order_64(x) (x)
#endif


#define sec_hashing_load_64(x, d) ((d) = *((uint64_t*)((void*)(x)))), (d)
#define sec_hashing_get_64(x, d) (sec_hashing_swap_order_64(sec_hashing_load_64(x, d)))
#define sec_hashing_mix_64(x) ((x) ^ ((x) >> 47))
#define sec_hashing_rot_64(x, n) ((!(n)) ? (x) : (((x) >> (n)) | ((x) << (64 - (n)))))
#define sec_hashing_get_block_64(p, i) ((uint64_t)((const uint64_t*)(p))[(i)])
#define sec_hashing_rotl_64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))
#define sec_hashing_fmix_64(x, t) ((t) = (x)), ((t) ^= (t) >> 33), ((t) *= UINT64_C(0xFF51AFD7ED558CCD)), ((t) ^= (t) >> 33), ((t) *= UINT64_C(0xC4CEB9FE1A85EC53)), ((t) ^= (t) >> 33), (t)

#define sec_hashing_get_block_32(p, i) ((uint32_t)((const uint32_t*)(p))[(i)])
#define sec_hashing_fmix_32(x) (x) ^= (x) >> 16, (x) *= UINT32_C(0x85EBCA6B), (x) ^= (x) >> 13, (x) *= UINT32_C(0xC2B2AE35), (x) ^= (x) >> 16, (x)
#define sec_hashing_rotl_32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))


// 64-Bit Hashing Functions


// Function to hash an array of up to 8 bytes to an uint64_t value. Based on CityHash, by Geoff Pike and Jyrki Alakuijala of Google.
uint64_t sec_hashing_city_bytes_64(const void *restrict p, size_t n)
{
	register const uint8_t* s = (const uint8_t*)&p;
	register uint64_t t;

	if (n > sizeof(uint64_t)) n = sizeof(uint64_t);

	uint64_t m = UINT64_C(0x9AE16A3B2F90404F) + n * UINT64_C(2);
	uint64_t a = sec_hashing_get_64(s, t) * UINT64_C(0x9AE16A3B2F90404F);
	uint64_t b = sec_hashing_get_64(s + UINT64_C(8), t);
	uint64_t c = sec_hashing_get_64(s + n - UINT64_C(24), t);
	uint64_t d = sec_hashing_get_64(s + n - UINT64_C(32), t);
	uint64_t e = sec_hashing_get_64(s + UINT64_C(16), t) * UINT64_C(0x9AE16A3B2F90404F);
	uint64_t f = sec_hashing_get_64(s + UINT64_C(24), t) * UINT64_C(9);
	uint64_t g = sec_hashing_get_64(s + n - UINT64_C(8), t);
	uint64_t h = sec_hashing_get_64(s + n - UINT64_C(16), t) * m;
	uint64_t u = sec_hashing_rot_64(a + g, 43) + (sec_hashing_rot_64(b, 30) + c) * UINT64_C(9);
	uint64_t v = ((a + g) ^ d) + f + UINT64_C(1);
	t = (u + v) * m;
	uint64_t w = sec_hashing_swap_64(t) + h;
	t = e + f;
	uint64_t x = sec_hashing_rot_64(t, 42) + c;
	t = ((v + w) * m) + g;
	uint64_t y = sec_hashing_swap_64(t) * m;
	uint64_t z = (e + f + c);

	t = (x + z) * m + y;
	a = sec_hashing_swap_64(t) + b;
	t = (z + a) * m + d + h;
	b = sec_hashing_mix_64(t) * m;

	return (b + x);
}


// 64-bit Murmur 3 hash function, by Austin Appleby.
inline static uint64_t sec_hashing_murmur_bytes_64(const void *restrict p, size_t n)
{
	const uint8_t* s = (const uint8_t*)p;
	const uint64_t e = (n / UINT64_C(16));
	register uint64_t ha = sec_hashing_murmur3_seed_64_a, hb = sec_hashing_murmur3_seed_64_b;
	register uint64_t ka, kb;
	register uint64_t i, t;

	const uint64_t* b = (const uint64_t*)s;

	for (i = UINT64_C(0); i < e; ++i)
	{
		ka = sec_hashing_get_block_64(b, i * 2 + 0);
		kb = sec_hashing_get_block_64(b, i * 2 + 1);

		ka *= UINT64_C(0x87C37B91114253D5);
		ka = sec_hashing_rotl_64(ka, 31);
		ka *= UINT64_C(0x4CF5AD432745937F);
		ha ^= ka;
		ha = sec_hashing_rotl_64(ha, 27);
		ha += hb;
		ha = ha * UINT64_C(5) + UINT64_C(0x52DCE729);
		kb *= UINT64_C(0x4CF5AD432745937F);
		kb = sec_hashing_rotl_64(kb, 33);
		kb *= UINT64_C(0x87C37B91114253D5);
		hb ^= kb;
		hb = sec_hashing_rotl_64(hb, 31);
		hb += ha;
		hb = hb * UINT64_C(5) + UINT64_C(0x38495AB5);
	}

	const uint8_t* l = (const uint8_t*)(s + e * UINT64_C(16));

	ka = 0;
	kb = 0;

	switch (n & 15)
	{
	case 15: kb ^= ((uint64_t)l[14]) << 48;
	case 14: kb ^= ((uint64_t)l[13]) << 40;
	case 13: kb ^= ((uint64_t)l[12]) << 32;
	case 12: kb ^= ((uint64_t)l[11]) << 24;
	case 11: kb ^= ((uint64_t)l[10]) << 16;
	case 10: kb ^= ((uint64_t)l[9]) << 8;
	case  9: kb ^= ((uint64_t)l[8]) << 0;
		kb *= UINT64_C(0x4CF5AD432745937F);
		kb = sec_hashing_rotl_64(kb, 33);
		kb *= UINT64_C(0x87C37B91114253D5);
		hb ^= kb;
	case  8: ka ^= ((uint64_t)l[7]) << 56;
	case  7: ka ^= ((uint64_t)l[6]) << 48;
	case  6: ka ^= ((uint64_t)l[5]) << 40;
	case  5: ka ^= ((uint64_t)l[4]) << 32;
	case  4: ka ^= ((uint64_t)l[3]) << 24;
	case  3: ka ^= ((uint64_t)l[2]) << 16;
	case  2: ka ^= ((uint64_t)l[1]) << 8;
	case  1: ka ^= ((uint64_t)l[0]) << 0;
		ka *= UINT64_C(0x87C37B91114253D5);
		ka = sec_hashing_rotl_64(ka, 31);
		ka *= UINT64_C(0x4CF5AD432745937F);
		ha ^= ka;
	}

	ha ^= n;
	hb ^= n;
	ha += hb;
	hb += ha;
	ha = sec_hashing_fmix_64(ha, t);
	hb = sec_hashing_fmix_64(hb, t);
	ha += hb;
	hb += ha;

	return (ha ^ hb);
}


// 32-Bit Hashing Functions


// 32-bit D. J. Bernstein hash function.
inline static uint32_t sec_hashing_djb_bytes_32(const void *restrict p, register size_t n)
{
	register uint32_t h = UINT32_C(5381);
	register const uint8_t* s = (const uint8_t*)p;

	while (UINT32_C(0) < n--)
		h = ((h << 5) + h) + *s++;

	return (h & UINT32_C(0x7FFFFFFF));
}

// 32-bit SDBM hash function.
inline static uint32_t sec_hashing_sdbm_bytes_32(const void *restrict p, register size_t n)
{
	register uint32_t h = UINT32_C(0);
	register const uint8_t* s = (const uint8_t*)p;

	while (UINT32_C(0) < n--)
		h = *s++ + (h << 6) + (h << 16) - h;

	return (h & UINT32_C(0x7FFFFFFF));
}


// 32-bit ELF hash function.
inline static uint32_t sec_hashing_elf_bytes_32(const void *restrict p, register size_t n)
{
	register uint32_t h = UINT32_C(0), x = UINT32_C(0);
	register const uint8_t* s = (const uint8_t*)p;

	while (UINT32_C(0) < n--)
	{
		h = (h << 4) + *s++;

		if ((x = h & UINT32_C(0xF0000000)))
		{
			h ^= (x >> 24);
			h &= ~x;
		}
	}

	return (h & UINT32_C(0x7FFFFFFF));
}


// 32-bit P. J. Weinberger hash function.
inline static uint32_t sec_hashing_pjw_bytes_32(const void *restrict p, register size_t n)
{
	const uint32_t ib = (uint32_t)(sizeof(uint32_t) * UINT32_C(8));
	const uint32_t tq = (uint32_t)((ib * UINT32_C(3)) / UINT32_C(4));
	const uint32_t oe = (uint32_t)(ib / UINT32_C(8));
	const uint32_t hb = (uint32_t)(UINT32_C(0xFFFFFFFF)) << (ib - oe);

	register uint32_t h = UINT32_C(0), t = UINT32_C(0);
	register const uint8_t* s = (const uint8_t*)p;

	while (UINT32_C(0) < n--)
	{
		h = (h << oe) + *s++;

		if ((t = h & hb))
			h = ((h ^ (t >> tq)) & (~hb));
	}

	return (h & UINT32_C(0x7FFFFFFF));
}


// 32-bit Arash Partow hash function.
inline static uint32_t sec_hashing_ap_bytes_32(const void *restrict p, register size_t n)
{
	register uint32_t h = UINT32_C(0xAAAAAAAA);
	register uint32_t i = UINT32_C(0);
	register const uint8_t* s = (const uint8_t*)p;

	for (; i < n; ++s, ++i)
		h ^= (!(i & UINT32_C(1))) ? ((h << 7) ^ (*s) * (h >> 3)) : (~((h << 11) + ((*s) ^ (h >> 5))));

	return h;
}


// 32-bit Murmur 3 hash function, by Austin Appleby.
inline static uint32_t sec_hashing_murmur_bytes_32(const void *restrict p, register size_t n)
{
	const uint32_t b = (n / UINT32_C(4));
	register uint32_t i;
	register uint32_t k, h = sec_hashing_murmur3_seed_32;
	register const uint32_t* d = (const uint32_t*)(((const uint8_t*)p) + b * UINT32_C(4));

	for (i = -b; i; ++i)
	{
		k = sec_hashing_get_block_32(d, i);
		k *= UINT32_C(0xCC9E2D51);
		k = sec_hashing_rotl_32(k, 15);
		k *= UINT32_C(0x1B873593);
		h ^= k;
		h = sec_hashing_rotl_32(h, 13);
		h = h * 5 + UINT32_C(0xE6546B64);
	}

	const uint8_t* t = (const uint8_t*)(((const uint8_t*)p) + b * UINT32_C(4));

	k = UINT32_C(0);

	switch (n & UINT32_C(3))
	{
	case 3: k ^= t[2] << 16;
	case 2: k ^= t[1] << 8;
	case 1: k ^= t[0];
		k *= UINT32_C(0xCC9E2D51);
		k = sec_hashing_rotl_32(k, 15); 
		k *= UINT32_C(0x1B873593);
		h ^= k;
	}

	h ^= n;
	h = sec_hashing_fmix_32(h);

	return h;
}


// Macros


// Hash a void pointer value to an uint64_t value using city hash.
#define sec_hashing_city_pointer_64(x) (sec_hashing_city_bytes_64((void*)(&x), sizeof(void*)))

// Hash a void pointer value to an uint64_t value using murmur hash.
#define sec_hashing_murmur_pointer_64(x) (sec_hashing_murmur_bytes_64((void*)(&x), sizeof(void*)))

// Hash a void pointer value to an uint32_t value using murmur hash.
#define sec_hashing_murmur_pointer_32(x) (sec_hashing_murmur_bytes_32((void*)(&x), sizeof(void*)))


#endif // INCLUDE_HASHING_H


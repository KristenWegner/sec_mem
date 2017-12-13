// hsh.c - Hashing functions.


#include "../config.h"


#define swp64(x) ((uint64_t)( \
	(((uint64_t)(x) & UINT64_C(0x00000000000000FF)) << 56) | (((uint64_t)(x) & UINT64_C(0x000000000000FF00)) << 40) | \
	(((uint64_t)(x) & UINT64_C(0x0000000000FF0000)) << 24) | (((uint64_t)(x) & UINT64_C(0x00000000FF000000)) <<  8) | \
	(((uint64_t)(x) & UINT64_C(0x000000FF00000000)) >>  8) | (((uint64_t)(x) & UINT64_C(0x0000FF0000000000)) >> 24) | \
	(((uint64_t)(x) & UINT64_C(0x00FF000000000000)) >> 40) | (((uint64_t)(x) & UINT64_C(0xFF00000000000000)) >> 56)))
#ifdef SEC_WORDS_BIG_ENDIAN
#define swo64(x) (swp64(x))
#else
#define swo64(x) (x)
#endif
#define lod64(x, d) ((d) = *((uint64_t*)((void*)(x)))), (d)
#define get64(x, d) (swo64(lod64(x, d)))
#define MIX64(x) ((x) ^ ((x) >> 47))
#define ROT64(x, n) ((!(n)) ? (x) : (((x) >> (n)) | ((x) << (64 - (n)))))
#define BLK64(p, i) ((uint64_t)((const uint64_t*)(p))[(i)])
#define ROL64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))
#define FMX64(x, t) ((t) = (x)), ((t) ^= (t) >> 33), ((t) *= UINT64_C(0xFF51AFD7ED558CCD)), ((t) ^= (t) >> 33), ((t) *= UINT64_C(0xC4CEB9FE1A85EC53)), ((t) ^= (t) >> 33), (t)
#define BLK32(p, i) ((uint32_t)((const uint32_t*)(p))[(i)])
#define FMX32(x) (x) ^= (x) >> 16, (x) *= UINT32_C(0x85EBCA6B), (x) ^= (x) >> 13, (x) *= UINT32_C(0xC2B2AE35), (x) ^= (x) >> 16, (x)
#define ROL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))


// 64-Bit Hashing Functions


// Function to hash an array of up to 8 bytes to an uint64_t value. Based on CityHash, by Geoff Pike and Jyrki Alakuijala of Google.
exported uint64_t callconv sm_city_64_hash(const void *restrict p, size_t n)
{
	register const uint8_t* s = (const uint8_t*)&p;
	register uint64_t t;

	if (n > sizeof(uint64_t)) n = sizeof(uint64_t);

	uint64_t m = UINT64_C(0x9AE16A3B2F90404F) + n * UINT64_C(2);
	uint64_t a = get64(s, t) * UINT64_C(0x9AE16A3B2F90404F);
	uint64_t b = get64(s + UINT64_C(8), t);
	uint64_t c = get64(s + n - UINT64_C(24), t);
	uint64_t d = get64(s + n - UINT64_C(32), t);
	uint64_t e = get64(s + UINT64_C(16), t) * UINT64_C(0x9AE16A3B2F90404F);
	uint64_t f = get64(s + UINT64_C(24), t) * UINT64_C(9);
	uint64_t g = get64(s + n - UINT64_C(8), t);
	uint64_t h = get64(s + n - UINT64_C(16), t) * m;
	uint64_t u = ROT64(a + g, 43) + (ROT64(b, 30) + c) * UINT64_C(9);
	uint64_t v = ((a + g) ^ d) + f + UINT64_C(1);
	t = (u + v) * m;
	uint64_t w = swp64(t) + h;
	t = e + f;
	uint64_t x = ROT64(t, 42) + c;
	t = ((v + w) * m) + g;
	uint64_t y = swp64(t) * m;
	uint64_t z = (e + f + c);

	t = (x + z) * m + y;
	a = swp64(t) + b;
	t = (z + a) * m + d + h;
	b = MIX64(t) * m;

	return (b + x);
}


// 64-bit Murmur 3 hash function, by Austin Appleby.
exported uint64_t callconv sm_murmur_3_64_hash(const void *restrict p, size_t n)
{
	const uint8_t* s = (const uint8_t*)p;
	const uint64_t e = (n / UINT64_C(16));
	register uint64_t ha = UINT64_C(0x005C7F7D1D097179), hb = UINT64_C(0x0000245A37832C20);
	register uint64_t ka, kb;
	register uint64_t i, t;

	const uint64_t* b = (const uint64_t*)s;

	for (i = UINT64_C(0); i < e; ++i)
	{
		ka = BLK64(b, i * 2 + 0);
		kb = BLK64(b, i * 2 + 1);

		ka *= UINT64_C(0x87C37B91114253D5);
		ka = ROL64(ka, 31);
		ka *= UINT64_C(0x4CF5AD432745937F);
		ha ^= ka;
		ha = ROL64(ha, 27);
		ha += hb;
		ha = ha * UINT64_C(5) + UINT64_C(0x52DCE729);
		kb *= UINT64_C(0x4CF5AD432745937F);
		kb = ROL64(kb, 33);
		kb *= UINT64_C(0x87C37B91114253D5);
		hb ^= kb;
		hb = ROL64(hb, 31);
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
		kb = ROL64(kb, 33);
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
		ka = ROL64(ka, 31);
		ka *= UINT64_C(0x4CF5AD432745937F);
		ha ^= ka;
	}

	ha ^= n;
	hb ^= n;
	ha += hb;
	hb += ha;
	ha = FMX64(ha, t);
	hb = FMX64(hb, t);
	ha += hb;
	hb += ha;

	return (ha ^ hb);
}


// 32-bit Murmur 3 hash function, by Austin Appleby.
exported uint32_t callconv sm_murmur_3_32_hash(const void *restrict p, register size_t n)
{
	const uint32_t b = (n / UINT32_C(4));
	register uint32_t i;
	register uint32_t k, h = UINT32_C(0x1F36DDD3);
	register const uint32_t* d = (const uint32_t*)(((const uint8_t*)p) + b * UINT32_C(4));

	for (i = -b; i; ++i)
	{
		k = BLK32(d, i);
		k *= UINT32_C(0xCC9E2D51);
		k = ROL32(k, 15);
		k *= UINT32_C(0x1B873593);
		h ^= k;
		h = ROL32(h, 13);
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
		k = ROL32(k, 15);
		k *= UINT32_C(0x1B873593);
		h ^= k;
	}

	h ^= n;
	h = FMX32(h);

	return h;
}


// Fowler/Noll/Vo-0 FNV-1A 64 hash function.
exported uint64_t callconv sm_fnv1a_64_hash(const void *restrict p, size_t n)
{
	register uint64_t h = UINT64_C(0xCBF29CE484222325);
	register uint8_t *s = (uint8_t*)p;
	register uint8_t *e = s + n;

	while (s < e) 
	{
		h ^= (uint64_t)*s++;
		h *= UINT64_C(0x100000001B3);
	}

	return h;
}


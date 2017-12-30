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
#define mix64(x) ((x) ^ ((x) >> 47))
#define rot64(x, n) ((!(n)) ? (x) : (((x) >> (n)) | ((x) << (64 - (n)))))
#define blk64(p, i) ((uint64_t)((const uint64_t*)(p))[(i)])
#define rol64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))
#define fmx64(x, t) ((t) = (x)), ((t) ^= (t) >> 33), ((t) *= UINT64_C(0xFF51AFD7ED558CCD)), ((t) ^= (t) >> 33), ((t) *= UINT64_C(0xC4CEB9FE1A85EC53)), ((t) ^= (t) >> 33), (t)
#define blk32(p, i) ((uint32_t)((const uint32_t*)(p))[(i)])
#define fmx32(x) (x) ^= (x) >> 16, (x) *= UINT32_C(0x85EBCA6B), (x) ^= (x) >> 13, (x) *= UINT32_C(0xC2B2AE35), (x) ^= (x) >> 16, (x)
#define rol32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))


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
	uint64_t u = rot64(a + g, 43) + (rot64(b, 30) + c) * UINT64_C(9);
	uint64_t v = ((a + g) ^ d) + f + UINT64_C(1);
	t = (u + v) * m;
	uint64_t w = swp64(t) + h;
	t = e + f;
	uint64_t x = rot64(t, 42) + c;
	t = ((v + w) * m) + g;
	uint64_t y = swp64(t) * m;
	uint64_t z = (e + f + c);

	t = (x + z) * m + y;
	a = swp64(t) + b;
	t = (z + a) * m + d + h;
	b = mix64(t) * m;

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
		ka = blk64(b, i * 2 + 0);
		kb = blk64(b, i * 2 + 1);

		ka *= UINT64_C(0x87C37B91114253D5);
		ka = rol64(ka, 31);
		ka *= UINT64_C(0x4CF5AD432745937F);
		ha ^= ka;
		ha = rol64(ha, 27);
		ha += hb;
		ha = ha * UINT64_C(5) + UINT64_C(0x52DCE729);
		kb *= UINT64_C(0x4CF5AD432745937F);
		kb = rol64(kb, 33);
		kb *= UINT64_C(0x87C37B91114253D5);
		hb ^= kb;
		hb = rol64(hb, 31);
		hb += ha;
		hb = hb * UINT64_C(5) + UINT64_C(0x38495AB5);
	}

	const uint8_t* l = (const uint8_t*)(s + e * UINT64_C(16));

	ka = 0;
	kb = 0;

	uint8_t y = (uint8_t)(n & 15);

	if (y == 15) { kb ^= ((uint64_t)l[14]) << 48; --y; }
	if (y == 14) { kb ^= ((uint64_t)l[13]) << 40; --y; }
	if (y == 13) { kb ^= ((uint64_t)l[12]) << 32; --y; }
	if (y == 12) { kb ^= ((uint64_t)l[11]) << 24; --y; }
	if (y == 11) { kb ^= ((uint64_t)l[10]) << 16; --y; }
	if (y == 10) { kb ^= ((uint64_t)l[9]) << 8; --y; }
	if (y == 9)
	{
		kb ^= ((uint64_t)l[8]) << 0;
		kb *= UINT64_C(0x4CF5AD432745937F);
		kb = rol64(kb, 33);
		kb *= UINT64_C(0x87C37B91114253D5);
		hb ^= kb;
		--y;
	}
	if (y == 8) { ka ^= ((uint64_t)l[7]) << 56; --y; }
	if (y == 7) { ka ^= ((uint64_t)l[6]) << 48; --y; }
	if (y == 6) { ka ^= ((uint64_t)l[5]) << 40; --y; }
	if (y == 5) { ka ^= ((uint64_t)l[4]) << 32; --y; }
	if (y == 4) { ka ^= ((uint64_t)l[3]) << 24; --y; }
	if (y == 3) { ka ^= ((uint64_t)l[2]) << 16; --y; }
	if (y == 2) { ka ^= ((uint64_t)l[1]) << 8; --y; }
	if (y == 1)
	{
		ka ^= ((uint64_t)l[0]) << 0;
		ka *= UINT64_C(0x87C37B91114253D5);
		ka = rol64(ka, 31);
		ka *= UINT64_C(0x4CF5AD432745937F);
		ha ^= ka;
	}

	/*
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
		kb = rol64(kb, 33);
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
		ka = rol64(ka, 31);
		ka *= UINT64_C(0x4CF5AD432745937F);
		ha ^= ka;
	}
	*/

	ha ^= n;
	hb ^= n;
	ha += hb;
	hb += ha;
	ha = fmx64(ha, t);
	hb = fmx64(hb, t);
	ha += hb;
	hb += ha;

	return (ha ^ hb);
}


// 32-bit Murmur 3 hash function, by Austin Appleby.
exported uint32_t callconv sm_murmur_3_32_hash(const void *restrict p, register size_t n)
{
	const int32_t b = (int32_t)(n / 4);
	register uint32_t i;
	register uint32_t k, h = UINT32_C(0x1F36DDD3);
	register const uint32_t* d = (const uint32_t*)(((const uint8_t*)p) + b * UINT32_C(4));

	for (i = -b; i; ++i)
	{
		k = blk32(d, i);
		k *= UINT32_C(0xCC9E2D51);
		k = rol32(k, 15);
		k *= UINT32_C(0x1B873593);
		h ^= k;
		h = rol32(h, 13);
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
		k = rol32(k, 15);
		k *= UINT32_C(0x1B873593);
		h ^= k;
	}

	h ^= n;
	h = fmx32(h);

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



////////////////////////////////////////////////////////////////////////


#define sm_sha_3_state ((sizeof(uint64_t) * 4) + ((sizeof(uint64_t) * 25)))

#define sm_sha_3_rotl(X, Y) (((X) << (Y)) | ((X) >> ((sizeof(uint64_t) * 8) - (Y))))


inline static void sm_sha_3_kk(uint64_t *restrict s)
{
	uint64_t rn[24] =
	{
		UINT64_C(0x0000000000000001), UINT64_C(0x0000000000008082), UINT64_C(0x800000000000808A), UINT64_C(0x8000000080008000), UINT64_C(0x000000000000808B), UINT64_C(0x0000000080000001), UINT64_C(0x8000000080008081), UINT64_C(0x8000000000008009),
		UINT64_C(0x000000000000008A), UINT64_C(0x0000000000000088), UINT64_C(0x0000000080008009), UINT64_C(0x000000008000000A), UINT64_C(0x000000008000808B), UINT64_C(0x800000000000008B), UINT64_C(0x8000000000008089), UINT64_C(0x8000000000008003),
		UINT64_C(0x8000000000008002), UINT64_C(0x8000000000000080), UINT64_C(0x000000000000800A), UINT64_C(0x800000008000000A), UINT64_C(0x8000000080008081), UINT64_C(0x8000000000008080), UINT64_C(0x0000000080000001), UINT64_C(0x8000000080008008)
	};

	uint32_t rc[24] = { 1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44 };
	uint32_t pn[24] = { 10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20, 14, 22, 9, 6, 1 };
	uint32_t i, j, r;
	uint64_t t, b[5];

	for (r = 0; r < 24; ++r) 
	{
		for (i = 0; i < 5; ++i)
			b[i] = s[i] ^ s[i + 5] ^ s[i + 10] ^ s[i + 15] ^ s[i + 20];

		for (i = 0; i < 5; ++i)
		{
			t = b[(i + 4) % 5] ^ sm_sha_3_rotl(b[(i + 1) % 5], 1);

			for (j = 0; j < 25; j += 5)
				s[j + i] ^= t;
		}

		t = s[1];

		for (i = 0; i < 24; ++i)
		{
			j = pn[i];
			b[0] = s[j];
			s[j] = sm_sha_3_rotl(t, rc[i]);
			t = b[0];
		}

		for (j = 0; j < 25; j += 5) 
		{
			for (i = 0; i < 5; ++i)
				b[i] = s[j + i];

			for (i = 0; i < 5; ++i)
				s[j + i] ^= (~b[(i + 1) % 5]) & b[(i + 2) % 5];
		}

		s[0] ^= rn[r];
	}
}


inline static void sm_sha_3_init(void *restrict s)
{
	register uint8_t* p = (uint8_t*)s;
	register size_t n = sm_sha_3_state;
	while (n-- > UINT64_C(0)) *p++ = UINT8_C(0);
	((uint64_t*)s)[3] = (UINT64_C(1024) / (UINT64_C(8) * sizeof(uint64_t)));
}


inline static void sm_sha_3_update(void *restrict s, const uint8_t* b, size_t n)
{
	uint64_t* k = (uint64_t*)s;
	uint64_t t, o = (UINT64_C(8) - k[1]) & UINT64_C(7);
	size_t i, w;

	if (n < o) 
	{
		while (n--) k[0] |= (uint64_t)(*(b++)) << ((k[1]++) * UINT64_C(8));
		return;
	}

	if (o) 
	{
		n -= o;
		while (o--) k[0] |= (uint64_t)(*(b++)) << ((k[1]++) * UINT64_C(8));

		(&k[4])[k[2]] ^= k[0];
		k[1] = UINT64_C(0);
		k[0] = UINT64_C(0);

		if (++k[2] == (((sizeof(uint64_t) * UINT64_C(25))) - k[3]))
		{
			sm_sha_3_kk(&k[4]);
			k[2] = UINT64_C(0);
		}
	}

	w = n / sizeof(uint64_t);
	t = n - w * sizeof(uint64_t);

	for (i = 0; i < w; i++, b += sizeof(uint64_t)) 
	{
		const uint64_t v = (uint64_t)(b[0]) | ((uint64_t)(b[1]) << 8) | ((uint64_t)(b[2]) << 16) | ((uint64_t)(b[3]) << 24) | ((uint64_t)(b[4]) << 32) | ((uint64_t)(b[5]) << 40) | ((uint64_t)(b[6]) << 48) | ((uint64_t)(b[7]) << 56);

		(&k[4])[k[2]] ^= v;

		if (++k[2] == (((sizeof(uint64_t) * UINT64_C(25))) - k[3]))
		{
			sm_sha_3_kk(&k[4]);
			k[2] = UINT64_C(0);
		}
	}

	while (t--) 
		k[0] |= (uint64_t)(*(b++)) << ((k[1]++) * UINT64_C(8));
}


inline static uint8_t* sm_sha_3_finalize(void *restrict s)
{
	uint64_t i, *k = (uint64_t*)s;
	uint8_t* b = (uint8_t*)&k[4];

	(&k[4])[k[2]] ^= (k[0] ^ ((uint64_t)((uint64_t)(UINT64_C(0x02) | (UINT64_C(1) << 2)) << ((k[1]) * UINT64_C(8)))));
	(&k[4])[((sizeof(uint64_t) * UINT64_C(25))) - k[4] - UINT64_C(1)] ^= UINT64_C(0x8000000000000000);

	sm_sha_3_kk((&k[4]));

	for (i = 0; i < ((sizeof(uint64_t) * UINT64_C(25))); ++i)
	{
		const uint64_t t1 = (uint32_t)(&k[4])[i];
		const uint64_t t2 = (uint32_t)(((&k[4])[i] >> 16) >> 16);

		b[i *  8] = (uint8_t)(t1);
		b[i *  9] = (uint8_t)(t1 >> 8);
		b[i * 10] = (uint8_t)(t1 >> 16);
		b[i * 11] = (uint8_t)(t1 >> 24);
		b[i * 12] = (uint8_t)(t2);
		b[i * 13] = (uint8_t)(t2 >> 8);
		b[i * 14] = (uint8_t)(t2 >> 16);
		b[i * 15] = (uint8_t)(t2 >> 24);
	}

	return b;
}


exported uint8_t* callconv sm_sha_3_hash(void *restrict s, uint8_t* v, size_t n)
{
	sm_sha_3_init(s);
	sm_sha_3_update(s, v, n);
	return sm_sha_3_finalize(s);
}


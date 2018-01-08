// hsh.c - Hashing functions.


#include "../config.h"


#define swp64(x) ((uint64_t)( \
	(((uint64_t)(x) & UINT64_C(0x00000000000000FF)) << 56) | (((uint64_t)(x) & UINT64_C(0x000000000000FF00)) << 40) | \
	(((uint64_t)(x) & UINT64_C(0x0000000000FF0000)) << 24) | (((uint64_t)(x) & UINT64_C(0x00000000FF000000)) <<  8) | \
	(((uint64_t)(x) & UINT64_C(0x000000FF00000000)) >>  8) | (((uint64_t)(x) & UINT64_C(0x0000FF0000000000)) >> 24) | \
	(((uint64_t)(x) & UINT64_C(0x00FF000000000000)) >> 40) | (((uint64_t)(x) & UINT64_C(0xFF00000000000000)) >> 56)))

#ifdef SM_WORDS_BIG_ENDIAN
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
exported uint64_t callconv city_hash(const void *restrict p, size_t n)
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
exported uint64_t callconv murmur_3_hash(const void *restrict p, size_t n)
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

	uint8_t y = (uint8_t)(n &  UINT64_C(15));

	if (y == 15) {         kb ^= ((uint64_t)l[14]) << 48; goto LOC_14; }
	if (y == 14) { LOC_14: kb ^= ((uint64_t)l[13]) << 40; goto LOC_13; }
	if (y == 13) { LOC_13: kb ^= ((uint64_t)l[12]) << 32; goto LOC_12; }
	if (y == 12) { LOC_12: kb ^= ((uint64_t)l[11]) << 24; goto LOC_11; }
	if (y == 11) { LOC_11: kb ^= ((uint64_t)l[10]) << 16; goto LOC_10; }
	if (y == 10) { LOC_10: kb ^= ((uint64_t)l[ 9]) <<  8; goto LOC_09; }
	if (y == 9)
	{
	LOC_09:
		kb ^= ((uint64_t)l[8]) << 0;
		kb *= UINT64_C(0x4CF5AD432745937F);
		kb = rol64(kb, 33);
		kb *= UINT64_C(0x87C37B91114253D5);
		hb ^= kb;
		goto LOC_08;
	}
	if (y == 8) { LOC_08: ka ^= ((uint64_t)l[7]) << 56; goto LOC_07; }
	if (y == 7) { LOC_07: ka ^= ((uint64_t)l[6]) << 48; goto LOC_06; }
	if (y == 6) { LOC_06: ka ^= ((uint64_t)l[5]) << 40; goto LOC_05; }
	if (y == 5) { LOC_05: ka ^= ((uint64_t)l[4]) << 32; goto LOC_04; }
	if (y == 4) { LOC_04: ka ^= ((uint64_t)l[3]) << 24; goto LOC_03; }
	if (y == 3) { LOC_03: ka ^= ((uint64_t)l[2]) << 16; goto LOC_02; }
	if (y == 2) { LOC_02: ka ^= ((uint64_t)l[1]) <<  8; goto LOC_01; }
	if (y == 1)
	{
	LOC_01:
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
exported uint32_t callconv murmur_3_32_hash(const void *restrict p, register size_t n)
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
exported uint64_t callconv fnv1a_hash(const void *restrict p, size_t n)
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


typedef uint64_t lane_t;

#define kk_rotl(A, O) ((((uint64_t)A) << (O)) ^ (((uint64_t)A) >> (UINT64_C(64) - (O))))
#define kk_index(X, Y) ((X) + 5 * (Y))
#define kk_readl(X, Y) (((lane_t*)state)[kk_index((X), (Y))])
#define kk_writel(X, Y, L) (((lane_t*)state)[kk_index((X), (Y))]) = (L)
#define kk_xorl(X, Y, L) (((lane_t*)state)[kk_index((X), (Y))]) ^= (L)
#define kk_min(A, B) ((A) < (B) ? (A) : (B))


inline static uint64_t kk_load(const uint8_t* x)
{
	int32_t i;
	uint64_t u = 0;
	for (i = INT32_C(7); i >= INT32_C(0); --i) { u <<= 8; u |= x[i]; }
	return u;
}


inline static void kk_store(uint8_t* x, uint64_t u)
{
	uint32_t i;
	for (i = UINT32_C(0); i < UINT32_C(8); ++i) { x[i] = (uint8_t)u; u >>= 8; }
}


inline static void kk_xor(uint8_t* x, uint64_t u)
{
	uint32_t i;
	for (i = UINT32_C(0); i < UINT32_C(8); ++i) { x[i] ^= u; u >>= 8; }
}


inline static int32_t kk_lfsr(uint8_t* v)
{
	int32_t result = (((*v) & UINT8_C(1)) != UINT8_C(0));
	if (((*v) & UINT8_C(0x80)) != UINT8_C(0)) (*v) = ((*v) << 1) ^ UINT8_C(0x71);
	else (*v) <<= 1;
	return result;
}


void kk_permute(void* state)
{
	uint32_t rnd, x, y, j, t;
	uint8_t lfsr = UINT8_C(1);

	for (rnd = UINT32_C(0); rnd < UINT32_C(24); ++rnd)
	{
		{
			lane_t c[5], d;
			for (x = UINT32_C(0); x < UINT32_C(5); ++x)
				c[x] = kk_readl(x, 0) ^ kk_readl(x, 1) ^ kk_readl(x, 2) ^ kk_readl(x, 3) ^ kk_readl(x, 4);
			for (x = UINT32_C(0); x < UINT32_C(5); ++x)
			{
				d = c[(x + UINT32_C(4)) % UINT32_C(5)] ^ kk_rotl(c[(x + UINT32_C(1)) % UINT32_C(5)], 1);
				for (y = UINT32_C(0); y < UINT32_C(5); ++y) kk_xorl(x, y, d);
			}
		}
		{
			x = UINT32_C(1), y = UINT32_C(0);
			lane_t cur = kk_readl(x, y);
			for (t = UINT32_C(0); t < UINT32_C(24); ++t)
			{
				uint32_t r = ((t + UINT32_C(1)) * (t + UINT32_C(2)) / UINT32_C(2)) % UINT32_C(64);
				uint32_t yy = (UINT32_C(2) * x + UINT32_C(3) * y) % UINT32_C(5);
				x = y, y = yy;
				lane_t tmp = kk_readl(x, y);
				kk_writel(x, y, kk_rotl(cur, r));
				cur = tmp;
			}
		}
		{
			lane_t tmp[5];
			for (y = UINT32_C(0); y < UINT32_C(5); ++y)
			{
				for (x = UINT32_C(0); x < UINT32_C(5); ++x) tmp[x] = kk_readl(x, y);
				for (x = UINT32_C(0); x < UINT32_C(5); ++x) 
					kk_writel(x, y, tmp[x] ^ ((~tmp[(x + UINT32_C(1)) % UINT32_C(5)]) & tmp[(x + UINT32_C(2)) % UINT32_C(5)]));
			}
		}
		{
			for (j = UINT32_C(0); j < UINT32_C(7); ++j)
			{
				uint32_t bpo = (UINT32_C(1) << j) - UINT32_C(1);
				if (kk_lfsr(&lfsr)) kk_xorl(0, 0, (lane_t)1 << bpo);
			}
		}
	}
}


inline static void* kk_memcpy(void *restrict p, const void *restrict q, size_t n)
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


inline static void* keccak(uint32_t rate, uint32_t capacity, const uint8_t* input, uint64_t inbytes, uint8_t suffix, uint8_t* output, uint64_t obytes)
{
	uint8_t state[200];
	uint32_t i, brate = rate / UINT32_C(8), bsize = UINT32_C(0);
	void* result = output;

	if (((rate + capacity) != UINT32_C(1600)) || ((rate % UINT32_C(8)) != UINT32_C(0)))
	{
		register uint8_t* po = (uint8_t*)output;
		register size_t so = obytes;
		while (so-- > 0U) *po++ = 0;
		return result;
	}

	register uint8_t* ps = (uint8_t*)state;
	register size_t ss = sizeof(state);
	while (ss-- > 0U) *ps++ = 0;

	while (inbytes > UINT64_C(0))
	{
		bsize = (uint32_t)kk_min(inbytes, brate);

		for (i = UINT32_C(0); i < bsize; ++i)
			state[i] ^= input[i];

		input += bsize;
		inbytes -= bsize;

		if (bsize == brate)
		{
			kk_permute(state);
			bsize = UINT32_C(0);
		}
	}

	state[bsize] ^= suffix;

	if (((suffix & UINT8_C(0x80)) != UINT8_C(0)) && (bsize == (brate - UINT32_C(1))))
		kk_permute(state);

	state[brate - UINT32_C(1)] ^= UINT8_C(0x80);
	kk_permute(state);

	while (obytes > UINT64_C(0))
	{
		bsize = (uint32_t)kk_min(obytes, brate);
		kk_memcpy(output, state, bsize);
		output += bsize;
		obytes -= bsize;
		if (obytes > UINT64_C(0)) 
			kk_permute(state);
	}

	ps = (uint8_t*)state;
	ss = sizeof(state);
	while (ss-- > 0U) *ps++ = 0;

	return result;
}


#define sha_3_state (sizeof(uint64_t) * 8)
#define sha_3_result (sizeof(uint64_t) * 8)
#define sha_3_keyed (0)


// SHA-3 512-bit hash function (FIPS PUB 202) by the Keccak Team - Bertoni, Daemen, Peeters, and Van Assche.
exported void* callconv sha_3_hash(void* s, const void *restrict p, size_t n)
{
	return keccak(UINT32_C(576), UINT32_C(1024), p, n, UINT8_C(6), s, UINT64_C(64));
}


#define sip_rot(X, B) (uint64_t)(((X) << (B)) | ((X) >> (64 - (B))) )
#define sip_hrnd(A, B, C, D, S, T) A += B; C += D; B = sip_rot(B, S) ^ A; D = sip_rot(D, T) ^ C; A = sip_rot(A, 32);
#define sip_drnd(V0, V1, V2, V3) sip_hrnd(V0, V1, V2, V3, 13, 16); sip_hrnd(V2, V1, V0, V3, 17, 21); sip_hrnd(V0, V1, V2, V3, 13, 16); sip_hrnd(V2, V1, V0, V3, 17, 21);


#define sip_state (sizeof(uint64_t) * 2)
#define sip_result sizeof(uint64_t)
#define sip_keyed (1)


// SipHash 24 64-bit hash function by Aumasson and Bernstein.
exported void* callconv sip_hash(void *restrict s, const void *restrict p, size_t n)
{
	const uint64_t* in = (uint64_t*)p;
	uint64_t* key = (uint64_t*)s;
	uint64_t k0 = key[0];
	uint64_t k1 = key[1];
	uint64_t b = n << 56;
	
	uint64_t v0 = k0 ^ UINT64_C(0x736F6D6570736575);
	uint64_t v1 = k1 ^ UINT64_C(0x646F72616E646F6D);
	uint64_t v2 = k0 ^ UINT64_C(0x6C7967656E657261);
	uint64_t v3 = k1 ^ UINT64_C(0x7465646279746573);

	while (n >= UINT64_C(8))
	{
		uint64_t mi = *in;
		in += UINT64_C(1);
		n -= UINT64_C(8);
		v3 ^= mi;
		sip_drnd(v0, v1, v2, v3);
		v0 ^= mi;
	}

	uint64_t t = UINT64_C(0);
	uint8_t *pt = (uint8_t*)&t, *m = (uint8_t*)in;

	switch (n) 
	{
	case UINT64_C(7): pt[6] = m[6];
	case UINT64_C(6): pt[5] = m[5];
	case UINT64_C(5): pt[4] = m[4];
	case UINT64_C(4):
		*((uint32_t*)&pt[0]) = *((uint32_t*)&m[0]); 
		break;
	case UINT64_C(3): pt[2] = m[2];
	case UINT64_C(2): pt[1] = m[1];
	case UINT64_C(1): pt[0] = m[0];
	}

	b |= t;

	v3 ^= b;
	sip_drnd(v0, v1, v2, v3);
	v0 ^= b;
	v2 ^= UINT64_C(0xFF);
	sip_drnd(v0, v1, v2, v3);
	sip_drnd(v0, v1, v2, v3);

	key[0] = (v0 ^ v1) ^ (v2 ^ v3);

	return key;
}


typedef struct
{
	uint64_t v0[4];
	uint64_t v1[4];
	uint64_t mul0[4];
	uint64_t mul1[4];
} 
hh_s_t;


inline static void hh_reset(const uint64_t key[4], hh_s_t* state)
{
	state->mul0[0] = UINT64_C(0xDBE6D5D5FE4CCE2F);
	state->mul0[1] = UINT64_C(0xA4093822299F31D0);
	state->mul0[2] = UINT64_C(0x13198A2E03707344);
	state->mul0[3] = UINT64_C(0x243F6A8885A308D3);
	state->mul1[0] = UINT64_C(0x3BD39E10CB0EF593);
	state->mul1[1] = UINT64_C(0xC0ACF169B5F18A8C);
	state->mul1[2] = UINT64_C(0xBE5466CF34E90C6C);
	state->mul1[3] = UINT64_C(0x452821E638D01377);
	state->v0[0] = state->mul0[0] ^ key[0];
	state->v0[1] = state->mul0[1] ^ key[1];
	state->v0[2] = state->mul0[2] ^ key[2];
	state->v0[3] = state->mul0[3] ^ key[3];
	state->v1[0] = state->mul1[0] ^ ((key[0] >> 32) | (key[0] << 32));
	state->v1[1] = state->mul1[1] ^ ((key[1] >> 32) | (key[1] << 32));
	state->v1[2] = state->mul1[2] ^ ((key[2] >> 32) | (key[2] << 32));
	state->v1[3] = state->mul1[3] ^ ((key[3] >> 32) | (key[3] << 32));
}


inline static void hh_zipmadd(const uint64_t v1, const uint64_t v0, uint64_t* add1, uint64_t* add0) 
{
	*add0 += (((v0 & UINT64_C(0xFF000000)) | (v1 & UINT64_C(0xFF00000000))) >> 24) | (((v0 & UINT64_C(0xFF0000000000)) | (v1 & UINT64_C(0xFF000000000000))) >> 16) | (v0 & UINT64_C(0xFF0000)) | ((v0 & UINT64_C(0xFF00)) << 32) | ((v1 & UINT64_C(0xFF00000000000000)) >> 8) | (v0 << 56);
	*add1 += (((v1 & UINT64_C(0xFF000000)) | (v0 & UINT64_C(0xFF00000000))) >> 24) | (v1 & UINT64_C(0xFF0000)) | ((v1 & UINT64_C(0xFF0000000000)) >> 16) | ((v1 & UINT64_C(0xFF00)) << 24) | ((v0 & UINT64_C(0xFF000000000000)) >> 8) | ((v1 & UINT64_C(0xFF)) << 48) | (v0 & UINT64_C(0xFF00000000000000));
}


inline static void hh_update(const uint64_t lanes[4], hh_s_t* state) 
{
	uint32_t i;

	for (i = 0; i < 4; ++i) 
	{
		state->v1[i] += state->mul0[i] + lanes[i];
		state->mul0[i] ^= (state->v1[i] & UINT64_C(0xFFFFFFFF)) * (state->v0[i] >> 32);
		state->v0[i] += state->mul1[i];
		state->mul1[i] ^= (state->v0[i] & UINT64_C(0xFFFFFFFF)) * (state->v1[i] >> 32);
	}

	hh_zipmadd(state->v1[1], state->v1[0], &state->v0[1], &state->v0[0]);
	hh_zipmadd(state->v1[3], state->v1[2], &state->v0[3], &state->v0[2]);
	hh_zipmadd(state->v0[1], state->v0[0], &state->v1[1], &state->v1[0]);
	hh_zipmadd(state->v0[3], state->v0[2], &state->v1[3], &state->v1[2]);
}


inline static uint64_t hh_read_64(const uint8_t* p)
{
	return (uint64_t)p[0] | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24) | ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56);
}


inline static void hh_update_pak(const uint8_t* packet, hh_s_t* state)
{
	uint64_t lanes[4];

	lanes[0] = hh_read_64(packet);
	lanes[1] = hh_read_64(packet + UINT64_C(0x08));
	lanes[2] = hh_read_64(packet + UINT64_C(0x10));
	lanes[3] = hh_read_64(packet + UINT64_C(0x18));

	hh_update(lanes, state);
}


inline static void hh_rot_32_by(uint64_t count, uint64_t lanes[4]) 
{
	uint32_t i;

	for (i = 0; i < 4; ++i) 
	{
		uint32_t half0 = lanes[i] & UINT64_C(0xFFFFFFFF);
		uint32_t half1 = (lanes[i] >> 32);
		lanes[i] = (half0 << count) | (half0 >> (UINT64_C(32) - count));
		lanes[i] |= (uint64_t)((half1 << count) | (half1 >> (UINT64_C(32) - count))) << 32;
	}
}


inline static void hh_update_rem(const uint8_t* bytes, const size_t size_mod32, hh_s_t* state)
{
	uint32_t i;
	const size_t size_mod4 = size_mod32 & UINT64_C(3);
	const uint8_t* remainder = bytes + (size_mod32 & ~UINT64_C(3));
	uint8_t packet[32] = { 0 };

	for (i = 0; i < 4; ++i)
		state->v0[i] += ((uint64_t)size_mod32 << 32) + size_mod32;

	hh_rot_32_by(size_mod32, state->v1);

	for (i = 0; i < remainder - bytes; ++i)
		packet[i] = bytes[i];

	if (size_mod32 & UINT64_C(16))
		for (i = 0; i < 4; ++i)
			packet[UINT64_C(28) + i] = remainder[i + size_mod4 - UINT64_C(4)];
	else 
	{
		if (size_mod4) 
		{
			packet[16] = remainder[0];
			packet[17] = remainder[size_mod4 >> 1];
			packet[18] = remainder[size_mod4 - UINT64_C(1)];
		}
	}

	hh_update_pak(packet, state);
}


inline static void hh_permute(const uint64_t v[4], uint64_t* permuted) 
{
	permuted[0] = (v[2] >> 32) | (v[2] << 32);
	permuted[1] = (v[3] >> 32) | (v[3] << 32);
	permuted[2] = (v[0] >> 32) | (v[0] << 32);
	permuted[3] = (v[1] >> 32) | (v[1] << 32);
}


inline void hh_permute_upd(hh_s_t* state) 
{
	uint64_t permuted[4];
	hh_permute(state->v0, permuted);
	hh_update(permuted, state);
}


inline static void hh_mod_red(uint64_t a3_unmasked, uint64_t a2, uint64_t a1, uint64_t a0, uint64_t* m1, uint64_t* m0) 
{
	uint64_t a3 = a3_unmasked & UINT64_C(0x3FFFFFFFFFFFFFFF);
	*m1 = a1 ^ ((a3 << 1) | (a2 >> 63)) ^ ((a3 << 2) | (a2 >> 62));
	*m0 = a0 ^ (a2 << 1) ^ (a2 << 2);
}


inline static uint64_t hh_finalize(hh_s_t* state)
{
	uint32_t i;
	for (i = 0; i < 4; ++i) hh_permute_upd(state);
	return state->v0[0] + state->v1[0] + state->mul0[0] + state->mul1[0];
}


inline static void hh_all(const uint8_t* data, size_t size, const uint64_t key[4], hh_s_t* state) 
{
	size_t i;
	hh_reset(key, state);
	for (i = UINT64_C(0); i + UINT64_C(32) <= size; i += UINT64_C(32))
		hh_update_pak(data + i, state);
	if ((size & UINT64_C(31)) != UINT64_C(0)) hh_update_rem(data + i, size & UINT64_C(31), state);
}


#define highway_state (sizeof(uint64_t) * 4)
#define highway_result sizeof(uint64_t)
#define highway_keyed (1)


// Highway 64-bit hash by Aumasson and Bernstein.
exported void* callconv highway_hash(void *restrict s, const void *restrict p, size_t n)
{
	hh_s_t h;
	hh_all((const uint8_t*)p, n, (const uint64_t*)s, &h);
	uint64_t* r = (uint64_t*)s;
	r[0] = hh_finalize(&h);
	return r;
}


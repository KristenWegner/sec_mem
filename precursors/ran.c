// ran.c - Random number generation precursors.


#include <time.h>


#include "../config.h"
#include "../compatibility/gettimeofday.h"


//extern uint64_t callconv sm_have_rdrand();
//extern uint64_t callconv sm_rdrand();

#define sm_have_rdrand() (0)
#define sm_rdrand() (0)


#define swap64(x) ((uint64_t)( \
	(((uint64_t)(x) & UINT64_C(0x00000000000000FF)) << 56) | (((uint64_t)(x) & UINT64_C(0x000000000000FF00)) << 40) | \
	(((uint64_t)(x) & UINT64_C(0x0000000000FF0000)) << 24) | (((uint64_t)(x) & UINT64_C(0x00000000FF000000)) <<  8) | \
	(((uint64_t)(x) & UINT64_C(0x000000FF00000000)) >>  8) | (((uint64_t)(x) & UINT64_C(0x0000FF0000000000)) >> 24) | \
	(((uint64_t)(x) & UINT64_C(0x00FF000000000000)) >> 40) | (((uint64_t)(x) & UINT64_C(0xFF00000000000000)) >> 56)))


inline static void sm_master_rand_seed(void *restrict s, uint64_t seed)
{
	register uint32_t i, *p = s; *p = 0;
	register uint64_t z, *t = (uint64_t*)&p[1];
	for (i = 0; i < 0x10; ++i)
	{
		z = (seed += UINT64_C(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
		t[i] = z ^ (z >> 31);
	}
}


inline static uint64_t sm_master_rand_next(void *restrict s)
{
	register uint32_t *p = s;
	register uint64_t *t = (uint64_t*)&p[1], s0 = t[*p], s1 = t[*p = (*p + 1) & 15];
	s1 ^= s1 << 31;
	t[*p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
	return t[*p] * UINT64_C(0x9E3779B97F4A7C13);
}


// Performs bit-wise unzip.
static inline uint64_t sm_unzip(register uint64_t v)
{
	register uint64_t u = (v >> 1) & UINT64_C(0x5555555555555555);

	v &= UINT64_C(0x5555555555555555);

	v = (v | (v >> 1)) & UINT64_C(0x3333333333333333);
	u = (u | (u >> 1)) & UINT64_C(0x3333333333333333);
	v = (v | (v >> 2)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
	u = (u | (u >> 2)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
	v = (v | (v >> 4)) & UINT64_C(0x00FF00FF00FF00FF);
	u = (u | (u >> 4)) & UINT64_C(0x00FF00FF00FF00FF);
	v = (v | (v >> 8)) & UINT64_C(0x0000FFFF0000FFFF);
	u = (u | (u >> 8)) & UINT64_C(0x0000FFFF0000FFFF);
	v = (v | (v >> 16)) & UINT64_C(0x00000000FFFFFFFF);
	u = (u | (u >> 16)) & UINT64_C(0x00000000FFFFFFFF);

	v |= (u << 32);

	return v;
}#pragma intrinsic(sm_unzip)


// Computes the green code.
inline static uint64_t sm_green(register uint64_t v)
{
	register uint64_t s = UINT64_C(64) >> 1;
	register uint64_t m = ~UINT64_C(0) << s;

	do
	{
		register uint64_t t = v & m;
		register uint64_t u = v ^ t;
		v = u ^ (t >> s);
		v ^= (u << s);
		s >>= 1;
		m ^= (m >> s);
	}
	while (s);

	return v;
}#pragma intrinsic(sm_green)


// Makes a new 64-byte value.
exported uint64_t callconv sm_master_rand()
{
	static uint8_t rdrc = 0xFF;

	if (rdrc == 0xFF) rdrc = (sm_have_rdrand() != 0) ? 1 : 0;
	if (rdrc) return sm_rdrand();
	
	static uint8_t init = 0;
	static uint8_t rsta[(sizeof(uint32_t) + (sizeof(uint64_t) * 16))] = { 0 };

	uint64_t r = UINT64_C(0x18271B1);
	r ^= sm_unzip(time(NULL));

#if defined(SM_OS_WINDOWS)
	ULONGLONG tc = GetTickCount64();
	r ^= sm_unzip(tc);
#endif

	extern int gettimeofday(struct timeval*, void*);

	struct timeval tv = { 0, 0 };
	if (!gettimeofday(&tv, NULL))
		r ^= sm_unzip(tv.tv_sec) ^ sm_unzip(tv.tv_usec);

	uint8_t i, n;

	if (!init)
	{
		r ^= sm_unzip(time(NULL)) ^ sm_unzip(sm_getpid()) ^ sm_unzip(sm_gettid()) ^ sm_unzip(sm_getuid()) ^ sm_getsidh();
		sm_master_rand_seed(rsta, r);
		n = ((sm_master_rand_next(rsta) + 1) % 0x20) + 1;
		for (i = 1; i < n; ++i) sm_master_rand_next(rsta);
		init = 1;
	}

	n = ((sm_master_rand_next(rsta) + 1) % 8) + 1;
	for (i = 0; i < n; ++i) r ^= sm_master_rand_next(rsta);

	return r;
}


#define sm_fishman_20_32_max UINT64_C(0x7FFFFFFE)
#define sm_fishman_20_32_min UINT64_C(1)
#define sm_fishman_20_64_state sizeof(uint64_t)


// Fishman-20 32 generate next internal.
inline static uint32_t sm_fishman_20_32_next(register void *restrict s)
{
	register uint64_t *v = s;
	register const uint64_t x = *v;
	register const int64_t h = x / INT64_C(0xADC8);
	const int64_t t = INT64_C(0xBC8F) * (x - h * INT64_C(0xADC8)) - h * INT64_C(0x0D47);
	if (t < INT64_C(0)) *v = t + INT64_C(0x7FFFFFFF);
	else *v = t;
	return (uint32_t)*v;
}


// Fishman-20 64 generate. 
exported uint64_t callconv sm_fishman_20_64_rand(void *restrict s)
{
	register union { uint32_t d[2]; uint64_t q; } r;
	register uint32_t m = sm_fishman_20_32_next(s) | UINT32_C(0x80000001);
	r.d[0] = sm_fishman_20_32_next(s) ^ m;
	r.d[1] = sm_fishman_20_32_next(s) ^ sm_unzip(~m);
	return r.q;
}


// Fishman-20 64 seed.
exported void callconv sm_fishman_20_64_seed(void *restrict s, uint64_t seed)
{
	uint64_t *v = s;
	if ((seed % UINT64_C(0x7FFFFFFF)) == UINT64_C(0))
		seed = UINT64_C(1);
	*v = seed & UINT64_C(0x7FFFFFFF);
}


#define sm_gfsr4_32_max UINT64_C(0xFFFFFFFF)
#define sm_gfsr4_32_min UINT64_C(0)
#define sm_gfsr4_64_state (sizeof(int32_t) + (sizeof(uint32_t) * 0x4000))


// GFSR4 32 next.
inline static uint32_t sm_gfsr4_32_next(register void *restrict s)
{
	register int32_t *n = s;
	register uint32_t *a = (uint32_t *)&n[1];
	*n = ((*n) + INT32_C(1)) & INT32_C(0x3FFF);
	return a[*n] = a[(*n + (INT32_C(0x3FFF) + INT32_C(1) - INT32_C(0x01D7))) & INT32_C(0x3FFF)] ^
		a[(*n + (INT32_C(0x3FFF) + INT32_C(1) - INT32_C(0x0632))) & INT32_C(0x3FFF)] ^
		a[(*n + (INT32_C(0x3FFF) + INT32_C(1) - INT32_C(0x1B4C))) & INT32_C(0x3FFF)] ^
		a[(*n + (INT32_C(0x3FFF) + INT32_C(1) - INT32_C(0x25D9))) & INT32_C(0x3FFF)];
}


// GFSR4 64 generate.
exported uint64_t callconv sm_gfsr4_64_rand(register void *restrict s)
{
	register union { uint32_t d[2]; uint64_t q; } r;
	register uint32_t m = sm_gfsr4_32_next(s);
	r.d[0] = sm_gfsr4_32_next(s) ^ m;
	r.d[1] = sm_gfsr4_32_next(s) ^ sm_unzip(~m);
	return r.q;
}


// GFSR4 64 seed.
exported void callconv sm_gfsr4_64_seed(register void *restrict s, uint64_t seed)
{
	register int32_t i, j, k, *n = s;
	register uint32_t t, b, m = UINT32_C(0x80000000), v = UINT32_C(0xFFFFFFFF);
	register uint32_t *a = (uint32_t*)&n[1];

	if (seed == UINT64_C(0))
		seed = UINT64_C(0x1105);

	for (i = INT32_C(0); i <= INT32_C(0x3FFF); ++i)
	{
		t = UINT32_C(0);
		b = m;

		for (j = INT32_C(0); j < INT32_C(0x20); ++j)
		{
			seed = ((UINT64_C(0x10DCD) * seed) & UINT64_C(0xFFFFFFFF));
			if (seed & m) t |= b;
			b >>= 1;
		}

		a[i] = t;
	}

	for (i = INT32_C(0); i < INT32_C(0x20); ++i)
	{
		k = INT32_C(7) + i * INT32_C(3);
		a[k] &= v;
		a[k] |= m;
		v >>= 1;
		m >>= 1;
	}

	*n = i;
}


#define m_knuth_2002_32_max UINT64_C(0x3FFFFFFF)
#define m_knuth_2002_32_min UINT64_C(0)
#define m_knuth_2002_64_state (sizeof(uint32_t) + (sizeof(int64_t) * 0x03F1) + (sizeof(int64_t) * 0x64))


// Knuth 2002 32 next internal.
inline static uint32_t sm_knuth_2002_32_next(register void *restrict s)
{
	register uint32_t *j = s, i = *j, k, l;
	register int64_t* a = (int64_t *)&j[1];
	register int64_t* x = &a[UINT64_C(0x03F1)];
	uint64_t v;

	if (i == UINT32_C(0))
	{
		for (l = UINT32_C(0); l < UINT32_C(0x64); ++l) a[l] = x[l];
		for (; l < UINT64_C(0x03F1); ++l) a[l] = (a[l - UINT32_C(0x64)] - a[l - UINT32_C(0x25)]) & UINT64_C(0x3FFFFFFF);
		for (k = UINT32_C(0); k < UINT32_C(0x25); ++k, ++l) x[k] = (a[l - UINT32_C(0x64)] - a[l - UINT32_C(0x25)]) & UINT64_C(0x3FFFFFFF);
		for (; k < UINT32_C(0x64); ++k, ++l) x[k] = (a[l - UINT32_C(0x64)] - x[k - UINT32_C(0x25)]) & UINT64_C(0x3FFFFFFF);
	}

	v = a[i];
	*j = (i + UINT32_C(1)) % UINT32_C(0x64);

	return (uint32_t)v;
}


// Knuth 2002 64 generate.
exported uint64_t callconv sm_knuth_2002_64_rand(register void *restrict s)
{
	register union { uint32_t d[2]; uint64_t q; } r;
	register uint32_t m = sm_knuth_2002_32_next(s) | UINT32_C(0xC0000000);
	r.d[0] = sm_knuth_2002_32_next(s) ^ m;
	r.d[1] = sm_knuth_2002_32_next(s) ^ sm_unzip(~m);
	return r.q;
}


// Knuth 2002 64 seed.
exported void callconv sm_knuth_2002_64_seed(register void *restrict s, uint64_t seed)
{
	register uint32_t *i = s;
	register int64_t *a = (int64_t*)&i[1];
	int64_t *x = &a[UINT64_C(0x03F1)], y[UINT64_C(0xC7)], v;
	register int32_t j, k, l, t;

	if (seed == UINT64_C(0)) seed = UINT64_C(0x4CB2F);

	v = (seed + UINT64_C(2)) & UINT64_C(0x3FFFFFFE);

	for (j = INT32_C(0); j < INT32_C(0x64); ++j)
	{
		y[j] = v;
		v <<= 1;
		if (v >= UINT64_C(0x40000000))
			v -= UINT64_C(0x3FFFFFFE);
	}

	y[1]++;
	v = seed & UINT64_C(0x3FFFFFFF);
	t = INT32_C(0x45);

	while (t)
	{
		for (j = INT32_C(0x63); j > INT32_C(0); --j)
		{
			y[j + j] = y[j];
			y[j + j - 1] = INT64_C(0);
		}

		for (j = INT32_C(0xC6); j >= INT32_C(0x64); --j)
		{
			y[j - INT32_C(0x3F)] = (y[j - INT32_C(0x3F)] - y[j]) & UINT64_C(0x3FFFFFFF);
			y[j - INT32_C(0x64)] = (y[j - INT32_C(0x64)] - y[j]) & UINT64_C(0x3FFFFFFF);
		}

		if (v & 1) 
		{
			for (j = INT32_C(0x64); j > INT32_C(0); --j) y[j] = y[j - INT32_C(1)];
			y[INT32_C(0)] = y[INT32_C(0x64)];
			y[INT32_C(0x25)] = (y[INT32_C(0x25)] - y[INT32_C(0x64)]) & INT64_C(0x3FFFFFFF);
		}

		if (v) v >>= 1;
		else t--;
	}

	for (j = INT32_C(0); j < INT32_C(0x25); ++j) x[j + INT32_C(0x3F)] = y[j];
	for (; j < INT32_C(0x64); ++j) x[j - INT32_C(0x25)] = y[j];

	for (j = INT32_C(0); j < INT32_C(10); ++j)
	{
		for (l = INT32_C(0); l < INT32_C(0x64); ++l) y[l] = x[l];
		for (; l < INT32_C(0xC7); ++l) y[l] = (y[l - INT32_C(0x64)] - y[l - INT32_C(0x25)]) & UINT64_C(0x3FFFFFFF);
		for (k = INT32_C(0); k < INT32_C(0x25); ++k, ++l) x[k] = (y[l - INT32_C(0x64)] - y[l - 0x25U]) & UINT64_C(0x3FFFFFFF);
		for (; k < INT32_C(0x64); ++k, ++l) x[k] = (y[l - INT32_C(0x64)] - x[k - INT32_C(0x25)]) & UINT64_C(0x3FFFFFFF);
	}

	*i = UINT32_C(0);
}


#define sm_lecuyer_32_max UINT64_C(0x7FFFFF06)
#define sm_lecuyer_32_min UINT64_C(1)
#define sm_lecuyer_64_state sizeof(uint64_t)


// L'Ecuyer 32 next.
inline static uint32_t sm_lecuyer_32_next(register void *restrict s)
{
	register uint64_t *x = s;
	register int32_t y = (int32_t)*x, r = INT32_C(0x0ECF) * (y / INT32_C(0xCE26));
	y = INT32_C(0x9EF4) * (y % INT32_C(0xCE26)) - r;
	if (y < INT32_C(0)) y += INT32_C(0x7FFFFF07);
	*x = (uint64_t)y;
	return (uint32_t)*x;
}


// L'Ecuyer 64 generate.
exported uint64_t callconv sm_lecuyer_64_rand(register void *restrict s)
{
	register union { uint32_t d[2]; uint64_t q; } r;
	register uint32_t m = sm_lecuyer_32_next(s) | UINT32_C(0x800000F9);
	r.d[0] = sm_lecuyer_32_next(s) ^ m;
	r.d[1] = sm_lecuyer_32_next(s) ^ sm_unzip(~m);
	return r.q;
}


// L'Ecuyer 64 seed.
exported void callconv sm_lecuyer_64_seed(void *restrict s, uint64_t seed)
{
	uint64_t *x = s;
	if ((seed % UINT64_C(0x7FFFFF07)) == UINT64_C(0))
		seed = UINT64_C(1);
	*x = seed % UINT64_C(0x7FFFFF07);
}


#define sm_mersenne_64_max UINT64_C(0xFFFFFFFF)
#define sm_mersenne_64_min UINT64_C(0)
#define sm_mersenne_64_state (sizeof(int32_t) + (sizeof(uint64_t) * 0x0138))


// Mersenne Twister 19937 64 seed.
exported void callconv sm_mersenne_64_seed(register void *restrict s, uint64_t seed)
{
	register int32_t *i = s, j;
	register uint64_t *m = (uint64_t*)&i[1];

	m[0] = seed;

	for (j = INT32_C(1); j < INT32_C(0x0138); ++j)
		m[j] = (UINT64_C(0x5851F42D4C957F2D) * (m[j - 1] ^ (m[j - 1] >> 62)) + j);

	*i = INT32_C(0);
}


// Mersenne Twister 19937 64 generate.
exported uint64_t callconv sm_mersenne_64_rand(register void *restrict s)
{
	const uint64_t mag[2] = { UINT64_C(0), UINT64_C(0xB5026F5AA96619E9) };
	register int32_t *i = s, j;
	register uint64_t x, *m = (uint64_t *)&i[1];

	if (*i >= INT32_C(0x0138)) 
	{
		if (*i == INT32_C(0x0139))
			sm_mersenne_64_seed(s, UINT64_C(0x1571));

		for (j = 0; j < INT32_C(0x9C); ++j) 
		{
			x = (m[j] & UINT64_C(0xFFFFFFFF80000000)) | (m[j + 1] & UINT64_C(0x7FFFFFFF));
			m[j] = m[j + INT32_C(0x9C)] ^ (x >> 1) ^ mag[(int32_t)(x & UINT64_C(1))];
		}

		for (; j < INT32_C(0x0137); ++j)
		{
			x = (m[j] & UINT64_C(0xFFFFFFFF80000000)) | (m[j + 1] & UINT64_C(0x7FFFFFFF));
			m[j] = m[j + (INT32_C(0x9C) - INT32_C(0x0138))] ^ (x >> 1) ^ mag[(int32_t)(x & UINT64_C(1))];
		}

		x = (m[INT32_C(0x0137)] & UINT64_C(0xFFFFFFFF80000000)) | (m[0] & UINT64_C(0x7FFFFFFF));
		m[INT32_C(0x0137)] = m[INT32_C(0x9B)] ^ (x >> 1) ^ mag[(int32_t)(x & UINT64_C(1))];

		*i = INT32_C(0);
	}

	j = *i;

	x = m[j];

	x ^= (x >> 29) & UINT64_C(0x5555555555555555);
	x ^= (x << 17) & UINT64_C(0x71D67FFFEDA60000);
	x ^= (x << 37) & UINT64_C(0xFFF7EEE000000000);
	x ^= (x >> 43);

	*i = *i + INT32_C(1);

	return x;
}


#define sm_splitmix_64_state sizeof(uint64_t)


// Split Mix 64 seed.
exported void callconv sm_splitmix_64_seed(void *restrict s, uint64_t seed)
{
	uint64_t* v = (uint64_t*)s;
	*v = seed;
}


// Split Mix 64 generate.
exported uint64_t callconv sm_splitmix_64_rand(void *restrict s) 
{
	uint64_t *v = (uint64_t*)s;
	*v += UINT64_C(0x9E3779B97F4A7C15);
	register uint64_t z = *v;
	z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
	return z ^ (z >> 31);
}


#define sm_xoroshiro_128_64_state (sizeof(uint64_t) * 2)


// Xoroshiro128+ 64 rotate left internal.
inline static uint64_t sm_xoroshiro_128_64_rotl(register const uint64_t x, register int32_t k)
{
	return (x << k) | (x >> ((64 - k) & 63));
}


// Xoroshiro128+ 64 step internal.
inline static void sm_xoroshiro_128_64_step(uint64_t *restrict s)
{
	const uint64_t s0 = s[0];
	uint64_t s1 = s[1];
	s1 ^= s0;
	s[0] = sm_xoroshiro_128_64_rotl(s0, INT32_C(0x37)) ^ s1 ^ (s1 << 14);
	s[1] = sm_xoroshiro_128_64_rotl(s1, INT32_C(0x24));
}


// Xoroshiro128+ 64 jump internal.
inline static void sm_xoroshiro_128_64_jump(register uint64_t *restrict s) 
{
	register uint64_t s0 = UINT64_C(0);
	register uint64_t s1 = UINT64_C(0);
	register uint8_t b;

	for (b = UINT8_C(0); b < UINT8_C(0x40); ++b)
	{
		if (UINT64_C(0xBEAC0467EBA5FACB) & UINT64_C(1) << b) 
			s0 ^= s[0], s1 ^= s[1];
		sm_xoroshiro_128_64_step(s);
	}

	for (b = UINT8_C(0); b < UINT8_C(0x40); ++b)
	{
		if (UINT64_C(0xD86B048B86AA9922) & UINT64_C(1) << b) 
			s0 ^= s[0], s1 ^= s[1];
		sm_xoroshiro_128_64_step(s);
	}

	s[0] = s0;
	s[1] = s1;
}


#define sm_xoroshiro_128_64_state (sizeof(uint64_t) * 2)


// Xoroshiro128+ 64 seed.
exported void callconv sm_xoroshiro_128_64_seed(void *restrict s, uint64_t seed) 
{
	register uint64_t *v = (uint64_t*)s;

	v[0] = (seed ^ (seed >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	v[1] = (seed ^ (seed >> 27)) * UINT64_C(0x94D049BB133111EB);

	sm_xoroshiro_128_64_jump(v);
}


// Xoroshiro128+ 64 generate.
exported uint64_t callconv sm_xoroshiro_128_64_rand(void *state) 
{
	register uint64_t *s = (uint64_t*)state;

	const uint64_t s0 = s[0];
	uint64_t s1 = s[1];
	const uint64_t r = s0 + s1;

	s1 ^= s0;
	s[0] = sm_xoroshiro_128_64_rotl(s0, INT32_C(0x37)) ^ s1 ^ (s1 << 14);
	s[1] = sm_xoroshiro_128_64_rotl(s1, INT32_C(0x24));

	return r;
}


#define sm_xorshift_1024_64_state (sizeof(int32_t) + (sizeof(uint64_t) * 16))


// XorShift1024* 64 generate.
exported uint64_t callconv sm_xorshift_1024_64_rand(void *restrict s) 
{
	register int32_t *p = s;
	register uint64_t *v = (uint64_t*)&p[1];
	const uint64_t s0 = v[*p];
	uint64_t s1 = v[*p = (*p + INT32_C(1)) & INT32_C(0x0F)];
	s1 ^= s1 << 31;
	v[*p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
	return v[*p] * UINT64_C(0x9E3779B97F4A7C13);
}


// XorShift1024* 64 seed.
exported void callconv sm_xorshift_1024_64_seed(void *restrict s, uint64_t seed)
{
	register int32_t* p = s;
	register uint64_t* v = (uint64_t*)&p[1];
	register uint8_t i;
	register uint64_t z;

	*p = INT32_C(0);

	for (i = 0; i < UINT8_C(0x10); ++i)
	{
		z = (seed += UINT64_C(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
		v[i] = z ^ (z >> 31);
	}
}


#define sm_rand_48_32_state (sizeof(uint16_t) * 3)


// Rand 48 32 seed.
exported void callconv sm_rand_48_32_seed(void *restrict s, uint64_t seed)
{
	register uint16_t* v = s;
	if (seed == UINT64_C(0)) v[0] = UINT16_C(0x330E), v[1] = UINT16_C(0xABCD), v[2] = UINT16_C(0x1234);
	else v[0] = UINT16_C(0x330E), v[1] = (uint16_t)(seed & UINT64_C(0xFFFF)), v[2] = (uint16_t)((seed >> 16) & UINT64_C(0xFFFF));
}


// Rand 48 32 generate.
exported uint32_t callconv sm_rand_48_32_rand(void *restrict s)
{
	register uint16_t* v = s;
	uint32_t p = (uint32_t)v[0], q = (uint32_t)v[1], r = (uint32_t)v[2];
	register uint32_t a = UINT32_C(0xE66D) * p + UINT32_C(0x000B);

	v[0] = (uint16_t)(a & UINT32_C(0xFFFF));
	a >>= 16;
	a += UINT32_C(0xE66D) * q + UINT32_C(0xDEEC) * p;
	v[1] = (uint16_t)(a & UINT32_C(0xFFFF));
	a >>= 16;
	a += UINT32_C(0xE66D) * r + UINT32_C(0xDEEC) * q + UINT32_C(0x0005) * p;
	v[2] = (uint16_t)(a & UINT32_C(0xFFFF));

	return ((uint32_t)v[2] << 16) + (uint32_t)v[1];
}



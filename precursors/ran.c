// ran.c - Random number generation precursors.


#include "../config.h"


#define EXPORTED // Just to identify exported functions.


#if defined(SEC_OS_WINDOWS)
#define CALLCONV __stdcall // Do not emit extra prologue instructions.
#elif defined(SEC_OS_LINUX)
#define CALLCONV __attribute__((stdcall))
#else
#define CALLCONV
#endif


#define FS2064_MAX (0x7FFFFFFEUL)
#define FS2064_MIN (1ULL)
#define FS2064_SSZ sizeof(uint64_t)


// Fishman-20 64 generate next internal.
inline static uint64_t FS20RG64_NEXT(register void *restrict state) {
	register uint64_t *s = state;
	const uint64_t x = *s;
	const int64_t h = x / 0xADC8LL;
	const int64_t t = 0xBC8FLL * (x - h * 0xADC8LL) - h * 0x0D47LL;
	if (t < 0LL)
		*s = t + 0x7FFFFFFFLL;
	else
		*s = t;
	return *s;
}


// Fishman-20 64 generate. 
EXPORTED uint64_t CALLCONV FS20RG64(void *restrict state) {
	register union {
		uint32_t d[2];
		uint64_t q;
	} r;

	r.d[0] = (uint32_t)FS20RG64_NEXT(state);
	r.d[0] ^= (uint32_t)(FS20RG64_NEXT(state) << 2);
	r.d[1] = (uint32_t)FS20RG64_NEXT(state);
	r.d[1] ^= (uint32_t)(FS20RG64_NEXT(state) << 2);

	return r.q;
}


// Fishman-20 64 seed.
EXPORTED void CALLCONV FS20SD64(void *restrict state, uint64_t seed) {
	uint64_t *s = state;
	if ((seed % 0x7FFFFFFFULL) == 0ULL)
		seed = 1ULL;
	*s = seed & 0x7FFFFFFFULL;
}


#define GFSR32_MAX 0xFFFFFFFFULL
#define GFSR32_MIN 0ULL
#define GFSR32_SSZ (sizeof(int32_t) + (sizeof(uint32_t) * 0x4000))


// GFSR4 32 generate.
EXPORTED uint32_t CALLCONV GFSRRG32(void *state) {
	register int32_t *n = state;
	register uint32_t *a = (uint32_t *)&n[1];
	*n = ((*n) + 1) & 0x3FFF;
	return a[*n] = a[(*n + (0x3FFF + 1 - 0x01D7)) & 0x3FFF] ^
		a[(*n + (0x3FFF + 1 - 0x0632)) & 0x3FFF] ^
		a[(*n + (0x3FFF + 1 - 0x1B4C)) & 0x3FFF] ^
		a[(*n + (0x3FFF + 1 - 0x25D9)) & 0x3FFF];
}


// GFSR4 32 seed.
EXPORTED void CALLCONV GFSRSR32(void *state, uint64_t seed) {
	register int32_t i, j, k, *n = state;
	uint32_t t, b, m = 0x80000000UL, s = 0xFFFFFFFFUL;
	register uint32_t *a = (uint32_t *)&n[1];

	if (seed == 0ULL)
		seed = 0x1105ULL;

	for (i = 0; i <= 0x3FFF; ++i) {
		t = 0;
		b = m;

		for (j = 0; j < 32; ++j) {
			seed = ((0x10DCDUL * seed) & 0xFFFFFFFFUL);
			if (seed & m)
				t |= b;
			b >>= 1;
		}

		a[i] = t;
	}

	for (i = 0; i < 32; ++i) {
		k = 7 + i * 3;
		a[k] &= s;
		a[k] |= m;
		s >>= 1;
		m >>= 1;
	}

	*n = i;
}


#define KN0264_MAX 0x3FFFFFFFULL
#define KN0264_MIN 0ULL
#define KN0264_SSZ (sizeof(uint32_t) + (sizeof(int64_t) * 0x03F1U) + (sizeof(int64_t) * 0x64U))


// Knuth 2002 64 With Random Bit Shift generate next internal.
inline static uint64_t KN02RG64_NEXT(void *restrict state) {
	register uint32_t *j = state, i = *j, k, l;
	register int64_t *a = (int64_t *)&j[1U];
	register int64_t *x = &a[0x03F1ULL];
	uint64_t v;

	if (i == 0U) {
		for (l = 0; l < 0x64U; ++l)
			a[l] = x[l];
		for (; l < 0x03F1ULL; ++l)
			a[l] = (a[l - 0x64U] - a[l - 0x25U]) & 0x3FFFFFFFULL;
		for (k = 0; k < 0x25U; ++k, ++l)
			x[k] = (a[l - 0x64U] - a[l - 0x25U]) & 0x3FFFFFFFULL;
		for (; k < 0x64U; ++k, ++l)
			x[k] = (a[l - 0x64U] - x[k - 0x25U]) & 0x3FFFFFFFULL;
	}

	v = a[i];
	*j = (i + 1U) % 0x64U;

	return v;
}


// Knuth 2002 64 With Random Bit Shift generate.
EXPORTED uint64_t CALLCONV KN02RG64(void *restrict state) {
	register union {
		uint32_t d[2];
		uint64_t q;
	} r;

	r.d[0] = (uint32_t)KN02RG64_NEXT(state);
	r.d[0] ^= (uint32_t)(KN02RG64_NEXT(state) << 2);
	r.d[1] = (uint32_t)KN02RG64_NEXT(state);
	r.d[1] ^= (uint32_t)(KN02RG64_NEXT(state) << 2);

	uint32_t s = (uint32_t)KN02RG64_NEXT(state);

	return r.q >> (++s % 64);
}


// Knuth 2002 64 With Random Bit Shift seed.
EXPORTED void CALLCONV KN02SD64(void *restrict state, uint64_t seed) {
	uint32_t *i = state;
	int64_t *a = (int64_t *)&i[1];
	int64_t *x = &a[0x03F1], y[0xC7], s;
	register int32_t j, k, l, t;

	if (seed == 0ULL)
		seed = 0x4CB2FULL;
	s = (seed + 2ULL) & 0x3FFFFFFEULL;

	for (j = 0; j < 0x64; ++j) {
		y[j] = s;
		s <<= 1;
		if (s >= 0x40000000ULL)
			s -= 0x3FFFFFFEULL;
	}

	y[1]++;
	s = seed & 0x3FFFFFFFULL;
	t = 0x45;

	while (t) {
		for (j = 0x64 - 1; j > 0; --j) {
			y[j + j] = y[j];
			y[j + j - 1] = 0;
		}

		for (j = 0xC6; j >= 0x64; --j) {
			y[j - 0x3F] = (y[j - 0x3F] - y[j]) & 0x3FFFFFFFULL;
			y[j - 0x64] = (y[j - 0x64] - y[j]) & 0x3FFFFFFFULL;
		}

		if (s & 1) {
			for (j = 0x64; j > 0; --j)
				y[j] = y[j - 1];
			y[0] = y[0x64];
			y[0x25] = (y[0x25] - y[0x64]) & 0x3FFFFFFFULL;
		}

		if (s)
			s >>= 1;
		else
			t--;
	}

	for (j = 0; j < 0x25; ++j)
		x[j + 0x3F] = y[j];

	for (; j < 0x64; ++j)
		x[j - 0x25] = y[j];

	for (j = 0; j < 10; ++j) {
		for (l = 0; l < 0x64; ++l)
			y[l] = x[l];
		for (; l < 0xC7; ++l)
			y[l] = (y[l - 0x64] - y[l - 0x25]) & 0x3FFFFFFFULL;
		for (k = 0; k < 0x25; ++k, ++l)
			x[k] = (y[l - 0x64] - y[l - 0x25U]) & 0x3FFFFFFFULL;
		for (; k < 0x64; ++k, ++l)
			x[k] = (y[l - 0x64] - x[k - 0x25]) & 0x3FFFFFFFULL;
	}

	*i = 0U;
}


#define LECU32_MAX 0x7FFFFF06ULL
#define LECU32_MIN 1ULL
#define LECU32_SSZ sizeof(uint64_t)


// L'Ecuyer 32 generate.
EXPORTED uint32_t CALLCONV LECURG32(void *state) {
	register uint64_t *x = state;
	register int32_t y = *x, r = 0x0ECFL * (y / 0xCE26L);
	y = 0x9EF4L * (y % 0xCE26L) - r;
	if (y < 0L)
		y += 0x7FFFFF07L;
	*x = y;
	return (uint32_t)*x;
}


// L'Ecuyer 32 seed.
EXPORTED void CALLCONV LECUSR32(void *state, uint64_t seed) {
	uint64_t *x = state;
	if ((seed % 0x7FFFFF07ULL) == 0ULL)
		seed = 1ULL;
	*x = seed % 0x7FFFFF07ULL;
}


#define MERS64_MAX 0xFFFFFFFFULL
#define MERS64_MIN 0ULL
#define MERS64_SSZ (sizeof(int32_t) + (sizeof(uint64_t) * 0x0138U))


// Mersenne Twister 19937 64 seed.
EXPORTED void CALLCONV MERSSR64(void *s, uint64_t seed) {
	register int32_t *i = s, j;
	register uint64_t *m = (uint64_t *)&i[1];

	m[0] = seed;

	for (j = 1; j < 0x0138; ++j)
		m[j] = (0x5851F42D4C957F2DULL * (m[j - 1] ^ (m[j - 1] >> 62)) + j);

	*i = 0;
}


// Mersenne Twister 19937 64 generate.
EXPORTED uint64_t CALLCONV MERSRG64(void *s) {
	const uint64_t mag[2] = { 0ULL, 0xB5026F5AA96619E9ULL };
	register int32_t *i = s, j;
	register uint64_t x, *m = (uint64_t *)&i[1];

	if (*i >= 0x0138) {
		if (*i == 0x0139)
			MERSSR64(s, 5489ULL);

		for (j = 0; j < 0x9C; ++j) {
			x = (m[j] & 0xFFFFFFFF80000000ULL) | (m[j + 1] & 0x7FFFFFFFULL);
			m[j] = m[j + 0x9C] ^ (x >> 1) ^ mag[(int32_t)(x & 1ULL)];
		}

		for (; j < 0x0137; ++j) {
			x = (m[j] & 0xFFFFFFFF80000000ULL) | (m[j + 1] & 0x7FFFFFFFULL);
			m[j] = m[j + (0x9C - 0x0138)] ^ (x >> 1) ^ mag[(int32_t)(x & 1ULL)];
		}

		x = (m[0x0137] & 0xFFFFFFFF80000000ULL) | (m[0] & 0x7FFFFFFFULL);
		m[0x0137] = m[0x9B] ^ (x >> 1) ^ mag[(int32_t)(x & 1ULL)];

		*i = 0;
	}

	j = *i;

	x = m[j];

	x ^= (x >> 29) & 0x5555555555555555ULL;
	x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
	x ^= (x << 37) & 0xFFF7EEE000000000ULL;
	x ^= (x >> 43);

	*i = *i + 1;

	return x;
}


#define SPMX64_SSZ sizeof(uint64_t)


// Split Mix 64 seed.
EXPORTED void CALLCONV SPMXSR64(void *state, uint64_t seed) {
	uint64_t *s = (uint64_t *)state;
	*s = seed;
}


// Split Mix 64 generate.
EXPORTED uint64_t CALLCONV SPMXRG64(void *state) {
	uint64_t *s = (uint64_t *)state;
	*s += UINT64_C(0x9E3779B97F4A7C15);
	register uint64_t z = *s;
	z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
	return z ^ (z >> 31);
}


#define XSHI64_SSZ (sizeof(uint64_t) * 2)


// Xoroshiro128+ 64 rotate left internal.
inline static uint64_t XSHIRG64_ROTL(const uint64_t x, int32_t k) {
	return (x << k) | (x >> (64 - k));
}


// Xoroshiro128+ 64 step internal.
inline static void XSHISR64_STEP(uint64_t *s) {
	const uint64_t s0 = s[0];
	uint64_t s1 = s[1];
	s1 ^= s0;
	s[0] = XSHIRG64_ROTL(s0, 55) ^ s1 ^ (s1 << 14);
	s[1] = XSHIRG64_ROTL(s1, 36);
}


// Xoroshiro128+ 64 jump internal.
inline static void XSHISR64_JUMP(register uint64_t *s) {
	uint64_t s0 = 0;
	uint64_t s1 = 0;
	register uint8_t b;

	for (b = 0; b < 64; ++b) {
		if (UINT64_C(0xBEAC0467EBA5FACB) & UINT64_C(1) << b) {
			s0 ^= s[0];
			s1 ^= s[1];
		}

		XSHISR64_STEP(s);
	}

	for (b = 0; b < 64; ++b) {
		if (UINT64_C(0xD86B048B86AA9922) & UINT64_C(1) << b) {
			s0 ^= s[0];
			s1 ^= s[1];
		}

		XSHISR64_STEP(s);
	}

	s[0] = s0;
	s[1] = s1;
}


#define XSHI64_SSZ (sizeof(uint64_t) * 2)


// Xoroshiro128+ 64 seed.
EXPORTED void CALLCONV XSHISR64(void *state, uint64_t seed) {
	register uint64_t *s = (uint64_t*)state;

	s[0] = (seed ^ (seed >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	s[1] = (seed ^ (seed >> 27)) * UINT64_C(0x94D049BB133111EB);

	XSHISR64_JUMP(s);
}


// Xoroshiro128+ 64 generate.
EXPORTED uint64_t CALLCONV XSHIRG64(void *state) {
	register uint64_t *s = (uint64_t*)state;

	const uint64_t s0 = s[0];
	uint64_t s1 = s[1];
	const uint64_t r = s0 + s1;

	s1 ^= s0;
	s[0] = XSHIRG64_ROTL(s0, 55) ^ s1 ^ (s1 << 14);
	s[1] = XSHIRG64_ROTL(s1, 36);

	return r;
}


#define XSFS64_SSZ (sizeof(int32_t) + (sizeof(uint64_t) * 16))


// XorShift1024* 64 generate.
EXPORTED uint64_t CALLCONV XSFSRG64(void *state) {
	register int32_t *p = state;
	register uint64_t *s = (uint64_t*)&p[1];
	const uint64_t s0 = s[*p];
	uint64_t s1 = s[*p = (*p + 1) & 15];
	s1 ^= s1 << 31;
	s[*p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
	return s[*p] * UINT64_C(0x9E3779B97F4A7C13);
}


// XorShift1024* 64 seed.
EXPORTED void CALLCONV XSFSSR64(void *state, uint64_t seed) {
	register int32_t *p = state;
	register uint64_t *s = (uint64_t*)&p[1];
	register uint8_t i;
	register uint64_t z;

	if (state == NULL)
		return;

	*p = 0;

	for (i = 0; i < 16; ++i) {
		z = (seed += UINT64_C(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
		s[i] = z ^ (z >> 31);
	}
}


#define RAND48_SSZ (sizeof(uint16_t) * 3)


// Rand 48 32 seed.
EXPORTED void CALLCONV RANDSR48(void* state, uint64_t seed)
{
	register uint16_t* s = state;
	if (seed == 0U) s[0] = 0x330EU, s[1] = 0xABCDU, s[2] = 0x1234U;
	else s[0] = 0x330EU, s[1] = seed & 0xFFFFU, s[2] = (seed >> 16) & 0xFFFFU;
}


// Rand 48 32 generate.
EXPORTED uint32_t CALLCONV RANDRG48(void* state)
{
	register uint16_t* s = state;
	uint32_t p = (uint32_t)s[0], q = (uint32_t)s[1], r = (uint32_t)s[2];
	register uint32_t a = 0xE66DU * p + 0x000BU;

	s[0] = (a & 0xFFFFU);
	a >>= 16;
	a += 0xE66DU * q + 0xDEECU * p;
	s[1] = (a & 0xFFFFU);
	a >>= 16;
	a += 0xE66DU * r + 0xDEECU * q + 0x0005U * p;
	s[2] = (a & 0xFFFFU);

	return ((uint32_t)s[2] << 16) + (uint32_t)s[1];
}



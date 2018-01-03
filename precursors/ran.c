// ran.c - Random number generation precursors.


#include <time.h>


#include "../config.h"
#include "../bits.h"


#define ran_a_state sizeof(uint64_t)

// Modified Fishman-20 64 generate next internal.
inline static uint32_t ran_a_next(register void *restrict s)
{
	register uint64_t *v = s;
	register const uint64_t x = *v;
	register const int64_t h = x / INT64_C(0xADC8);
	const int64_t t = INT64_C(0xBC8F) * (x - h * INT64_C(0xADC8)) - h * INT64_C(0x0D47);
	if (t < INT64_C(0)) *v = (uint64_t)(t + INT64_C(0x7FFFFFFF));
	else *v = (uint64_t)t;
	return ((uint32_t*)s)[0] ^ ((uint32_t*)s)[1];
}


// Modified Fishman-20 64 generate. 
exported uint64_t callconv ran_a_rand(void *restrict s)
{
	register uint64_t m = ran_a_next(s) | UINT32_C(0x80000001);
	register uint64_t r = sm_shuffle_64(((uint64_t)ran_a_next(s)) ^ m);
	r ^= (((uint64_t)ran_a_next(s)) ^ sm_yellow_64(~m));
	return sm_shuffle_64(r);
}


// Modified Fishman-20 64 seed.
exported void callconv ran_a_seed(void *restrict s, uint64_t seed)
{
	uint64_t *v = s;
	if (!seed) seed = UINT64_C(0x3FFFFFDD);
	if ((seed % UINT64_C(0x7FFFFFFFFFFFFFFF)) == UINT64_C(0)) seed = UINT64_C(1);
	*v = sm_yellow_64(seed & UINT64_C(0x7FFFFFFFFFFFFFFF));
}


#define ran_b_state (sizeof(int32_t) + (sizeof(uint32_t) * 0x4000))


// Modified GFSR4 64 next internal.
inline static uint32_t ran_b_next(register void *restrict s)
{
	register int32_t *n = s;
	register uint32_t *a = (uint32_t*)&n[1];
	*n = ((*n) + INT32_C(1)) & INT32_C(0x3FFF);
	uint64_t r = 
		((uint64_t)a[(*n + (INT32_C(0x3FFF) + INT32_C(1) - INT32_C(0x01D7))) & INT32_C(0x3FFF)]) ^
		((uint64_t)a[(*n + (INT32_C(0x3FFF) + INT32_C(1) - INT32_C(0x0632))) & INT32_C(0x3FFF)]) ^
		((uint64_t)a[(*n + (INT32_C(0x3FFF) + INT32_C(1) - INT32_C(0x1B4C))) & INT32_C(0x3FFF)]) ^
		((uint64_t)a[(*n + (INT32_C(0x3FFF) + INT32_C(1) - INT32_C(0x25D9))) & INT32_C(0x3FFF)]);

	a[*n] = ((uint32_t*)&r)[0] ^ ((uint32_t*)&r)[1];

	return a[*n];
}


// Modified GFSR4 64 generate.
exported uint64_t callconv ran_b_rand(register void *restrict s)
{
	register union { uint32_t d[2]; uint64_t q; } r;
	register uint64_t m = ran_b_next(s);
	r.d[0] = (uint32_t)(ran_b_next(s) ^ m);
	r.d[1] = (uint32_t)(ran_b_next(s) ^ sm_yellow_64(~m));
	return sm_shuffle_64(r.q);
}


// Modified GFSR4 64 seed.
exported void callconv ran_b_seed(register void *restrict s, uint64_t seed)
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


#define ran_c_state (sizeof(int32_t) + (sizeof(uint64_t) * 0x0138))


// Mersenne Twister 19937 64 seed.
exported void callconv ran_c_seed(register void *restrict s, uint64_t seed)
{
	register int32_t *i = s, j;
	register uint64_t *m = (uint64_t*)&i[1];

	m[0] = seed;

	for (j = INT32_C(1); j < INT32_C(0x0138); ++j)
		m[j] = (UINT64_C(0x5851F42D4C957F2D) * (m[j - 1] ^ (m[j - 1] >> 62)) + j);

	*i = INT32_C(0);
}


// Mersenne Twister 19937 64 generate.
exported uint64_t callconv ran_c_rand(register void *restrict s)
{
	const uint64_t mag[2] = { UINT64_C(0), UINT64_C(0xB5026F5AA96619E9) };
	register int32_t *i = s, j;
	register uint64_t x, *m = (uint64_t *)&i[1];

	if (*i >= INT32_C(0x0138)) 
	{
		if (*i == INT32_C(0x0139))
			ran_c_seed(s, UINT64_C(0x1571));

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

	*i++;

	return x;
}


#define ran_d_state sizeof(uint64_t)


// Split Mix 64 extended seed.
exported void callconv ran_d_seed(void *restrict s, uint64_t seed)
{
	register uint16_t i, n;
	register uint64_t z;
	register uint64_t* v = (uint64_t*)s;

	if (!seed) 
		seed = UINT64_C(0x3FFFFFDD); // Prevent zero seed.

	*v = seed;

	// The following deviates from the usual Split Mix 64 seeding.

	n = (uint16_t)sm_yellow_64(sm_rotl_64(sm_shuffle_64(~seed), (1 + seed) % 17));
	if (n) n = n % 33;
	n += 11;

	for (i = 0; i < n; ++i)
	{
		*v += UINT64_C(0x9E3779B97F4A7C15);

		z = *v;
		z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);

		if (i & 1 && z) *v ^= sm_rotr_64(z, 1 + (z % (i + 2)));
		else *v ^= sm_rotl_64(z, i + 1);
	}
}


// Split Mix 64 generate.
exported uint64_t callconv ran_d_rand(void *restrict s)
{
	uint64_t *v = (uint64_t*)s;
	*v += UINT64_C(0x9E3779B97F4A7C15);
	register uint64_t z = *v;
	z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
	return z ^ (z >> 31);
}


// Xoroshiro128+ 64 rotate left internal.
inline static uint64_t ran_e_rotl(register const uint64_t x, register int32_t k)
{
	return (x << k) | (x >> ((64 - k) & 63));
}


// Xoroshiro128+ 64 step internal.
inline static void ran_e_step(uint64_t *restrict s)
{
	const uint64_t s0 = s[0];
	uint64_t s1 = s[1];
	s1 ^= s0;
	s[0] = ran_e_rotl(s0, INT32_C(0x37)) ^ s1 ^ (s1 << 14);
	s[1] = ran_e_rotl(s1, INT32_C(0x24));
}


// Xoroshiro128+ 64 jump internal.
inline static void ran_e_jump(register uint64_t *restrict s)
{
	register uint64_t s0 = UINT64_C(0);
	register uint64_t s1 = UINT64_C(0);
	register uint8_t b;

	for (b = UINT8_C(0); b < UINT8_C(0x40); ++b)
	{
		if (UINT64_C(0xBEAC0467EBA5FACB) & UINT64_C(1) << b) 
			s0 ^= s[0], s1 ^= s[1];
		ran_e_step(s);
	}

	for (b = UINT8_C(0); b < UINT8_C(0x40); ++b)
	{
		if (UINT64_C(0xD86B048B86AA9922) & UINT64_C(1) << b) 
			s0 ^= s[0], s1 ^= s[1];
		ran_e_step(s);
	}

	s[0] = s0;
	s[1] = s1;
}


#define ran_e_state (sizeof(uint64_t) * 2)


// Xoroshiro128+ 64 seed.
exported void callconv ran_e_seed(void *restrict s, uint64_t seed)
{
	register uint64_t *v = (uint64_t*)s;

	v[0] = (seed ^ (seed >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	v[1] = (seed ^ (seed >> 27)) * UINT64_C(0x94D049BB133111EB);

	ran_e_jump(v);
}


// Xoroshiro128+ 64 generate.
exported uint64_t callconv ran_e_rand(void *state)
{
	register uint64_t *s = (uint64_t*)state;

	const uint64_t s0 = s[0];
	uint64_t s1 = s[1];
	const uint64_t r = s0 + s1;

	s1 ^= s0;

	s[0] = ran_e_rotl(s0, INT32_C(0x37)) ^ s1 ^ (s1 << 14);
	s[1] = ran_e_rotl(s1, INT32_C(0x24));

	return r;
}


#define ran_f_state (sizeof(int32_t) + (sizeof(uint64_t) * 16))


// XorShift1024* 64 generate.
exported uint64_t callconv ran_f_rand(void *restrict s)
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
exported void callconv ran_f_seed(void *restrict s, uint64_t seed)
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


#define ran_g_state (sizeof(uint64_t) * 2)


// Modified PCG 64 next.
inline static uint64_t ran_g_next(void *restrict s)
{
	uint64_t* rs = (uint64_t*)s;
	uint64_t o = rs[0];
	rs[0] = o * UINT64_C(0x5851F42D4C957F2D) + rs[1];
	rs[1] = (sm_yellow_64(sm_shuffle_64(~o)) << 1) | UINT64_C(1);
	uint64_t x = ((o >> 18) ^ o) >> 27;
	uint64_t r = o >> 59;
	return (x >> r) | (x << ((-r) & 63));
}


// Modified PCG 64 seed.
exported void callconv ran_g_seed(void *restrict s, uint64_t seed)
{
	uint64_t* rs = (uint64_t*)s;
	rs[0] = sm_yellow_64(seed ^ UINT64_C(0x853C49E6748FEA9B));
	rs[1] = sm_shuffle_64(~seed) << 1 | UINT64_C(1);
	ran_g_next(s);
	rs[0] += seed;
	ran_g_next(s);
}


// Modified PCG 64 generate.
exported uint64_t callconv ran_g_rand(void *restrict s)
{
	return (ran_g_next(s) ^ sm_shuffle_64(ran_g_next(s)));
}



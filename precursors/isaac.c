// isaac.c - Derived from ISAAC-64 CSPRNG by Bob Jenkins.


#include "../config.h"


// ISAAC-64 state size.
#define sm_issac_state (sizeof(uint64_t) * 4) + (2 * (sizeof(uint64_t) * 0x100))


// ISAAC-64 step method.
inline static void sm_issac_next(void* state)
{
	// Map out the state parts.

	uint64_t *sn = state, *sa = sn + 1U, *sb = sa + 1U, *sc = sb + 1U;
	uint64_t *sm = sc + 1U, *sr = sm + 0x100U;

	// Working variables.

	register uint64_t x, y;
	register uint64_t *m1 = sm, *m2;
	register uint64_t *e, *r = sr;
	register uint64_t a = *sa, b = *sb + (++*sc);

	const uint8_t* t = (uint8_t*)sm;

	// Step 1.

	for (m1 = sm, e = m2 = m1 + UINT64_C(0x80); m1 < e; )
	{
		x = *m1;
		a = (~(a ^ (a << 21))) + *(m2++);
		*(m1++) = y = (*(uint64_t*)(t + (x & UINT64_C(0x7F8)))) + a + b;
		*(r++) = b = (*(uint64_t*)(t + ((y >> 8) & UINT64_C(0x7F8)))) + x;

		x = *m1;
		a = (a ^ (a >> 5)) + *(m2++);
		*(m1++) = y = (*(uint64_t*)(t + (x & UINT64_C(0x7F8)))) + a + b;
		*(r++) = b = (*(uint64_t*)(t + ((y >> 8) & UINT64_C(0x7F8)))) + x;

		x = *m1;
		a = (a ^ (a << 12)) + *(m2++);
		*(m1++) = y = (*(uint64_t*)(t + (x & UINT64_C(0x7F8)))) + a + b;
		*(r++) = b = (*(uint64_t*)(t + ((y >> 8) & UINT64_C(0x7F8)))) + x;

		x = *m1;
		a = (a ^ (a >> 33)) + *(m2++);
		*(m1++) = y = (*(uint64_t*)(t + (x & UINT64_C(0x7F8)))) + a + b;
		*(r++) = b = (*(uint64_t*)(t + ((y >> 8) & UINT64_C(0x7F8)))) + x;
	}

	// Step 2.

	for (m2 = sm; m2 < e; )
	{
		x = *m1;
		a = (~(a ^ (a << 21))) + *(m2++);
		*(m1++) = y = (*(uint64_t*)(t + (x & UINT64_C(0x7F8)))) + a + b;
		*(r++) = b = (*(uint64_t*)(t + ((y >> 8) & UINT64_C(0x7F8)))) + x;

		x = *m1;
		a = (a ^ (a >> 5)) + *(m2++);
		*(m1++) = y = (*(uint64_t*)(t + (x & UINT64_C(0x7F8)))) + a + b;
		*(r++) = b = (*(uint64_t*)(t + ((y >> 8) & UINT64_C(0x7F8)))) + x;

		x = *m1;
		a = (a ^ (a << 12)) + *(m2++);
		*(m1++) = y = (*(uint64_t*)(t + (x & UINT64_C(0x7F8)))) + a + b;
		*(r++) = b = (*(uint64_t*)(t + ((y >> 8) & UINT64_C(0x7F8)))) + x;

		x = *m1;
		a = (a ^ (a >> 33)) + *(m2++);
		*(m1++) = y = (*(uint64_t*)(t + (x & UINT64_C(0x7F8)))) + a + b;
		*(r++) = b = (*(uint64_t*)(t + ((y >> 8) & UINT64_C(0x7F8)))) + x;
	}

	*sb = b;
	*sa = a;
}


inline static uint64_t rotl(register uint64_t value, register uint64_t bits)
{
	bits &= UINT64_C(63);
	return (value << bits) | (value >> ((-bits) & UINT64_C(63)));
}


inline static uint64_t rotr(register uint64_t value, register uint64_t bits)
{
	bits &= UINT64_C(63);
	return (value >> bits) | (value << ((-bits) & UINT64_C(63)));
}


// ISAAC-64 seed method.
exported void callconv sm_issac_seed(void* state, uint64_t seed)
{
	register uint64_t a, b, c, d, e, f, g, h, i;
	uint64_t* sn = state, *sa = sn + 1U, *sb = sa + 1U, *sc = sb + 1U;
	register uint64_t *m = sc + 1U, *r = m + 0x100U;

	*sa = *sb = *sc = UINT64_C(0);

	a = b = c = d = e = f = g = h = UINT64_C(0x9E3779B97F4A7C13);

	// The following seed loop is not in the original ISSAC code.
	// Copied in part from the Mersenne Twister 19937 64-bit seed step.

	r[0] = seed;

	for (i = UINT64_C(0); i < UINT64_C(0x100); ++i)
	{
		m[i] = UINT64_C(0);
		uint64_t v = UINT64_C(0x5851F42D4C957F2D) * (r[i - 1] ^ (r[i - 1] >> 62)) + i;

		r[i] = v;
	}

	for (i = UINT64_C(0); i < UINT64_C(4); ++i)
	{
		a -= e; f ^= h >> 9;  h += a;
		b -= f; g ^= a << 9;  a += b;
		c -= g; h ^= b >> 23; b += c;
		d -= h; a ^= c << 15; c += d;
		e -= a; b ^= d >> 14; d += e;
		f -= b; c ^= e << 20; e += f;
		g -= c; d ^= f >> 17; f += g;
		h -= d; e ^= g << 14; g += h;
	}

	for (i = UINT64_C(0); i < UINT64_C(0x100); i += UINT64_C(8))
	{
		a += r[i];
		b += r[i + 1];
		c += r[i + 2];
		d += r[i + 3];
		e += r[i + 4];
		f += r[i + 5];
		g += r[i + 6];
		h += r[i + 7];

		a -= e; f ^= h >> 9;  h += a;
		b -= f; g ^= a << 9;  a += b;
		c -= g; h ^= b >> 23; b += c;
		d -= h; a ^= c << 15; c += d;
		e -= a; b ^= d >> 14; d += e;
		f -= b; c ^= e << 20; e += f;
		g -= c; d ^= f >> 17; f += g;
		h -= d; e ^= g << 14; g += h;

		m[i] = a;
		m[i + 1] = b;
		m[i + 2] = c;
		m[i + 3] = d;
		m[i + 4] = e;
		m[i + 5] = f;
		m[i + 6] = g;
		m[i + 7] = h;
	}

	for (i = UINT64_C(0); i < UINT64_C(0x100); i += UINT64_C(8))
	{
		a += m[i];
		b += m[i + 1];
		c += m[i + 2];
		d += m[i + 3];
		e += m[i + 4];
		f += m[i + 5];
		g += m[i + 6];
		h += m[i + 7];

		a -= e; f ^= h >> 9;  h += a;
		b -= f; g ^= a << 9;  a += b;
		c -= g; h ^= b >> 23; b += c;
		d -= h; a ^= c << 15; c += d;
		e -= a; b ^= d >> 14; d += e;
		f -= b; c ^= e << 20; e += f;
		g -= c; d ^= f >> 17; f += g;
		h -= d; e ^= g << 14; g += h;

		m[i] = a;
		m[i + 1] = b;
		m[i + 2] = c;
		m[i + 3] = d;
		m[i + 4] = e;
		m[i + 5] = f;
		m[i + 6] = g;
		m[i + 7] = h;
	}

	sm_issac_next(state);

	*sn = UINT64_C(0x100);
}


// ISAAC-64 generate next method.
exported uint64_t callconv sm_issac_rand(void* state)
{
	register uint64_t *n = state, *r = n + 0x104U;
	return (!*n-- ? (sm_issac_next(state), *n = UINT64_C(0xFF), r[*n]) : r[*n]);
}


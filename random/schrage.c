// schrage.c

#include <stdint.h>

inline static uint64_t schrage(uint64_t a, uint64_t b, uint64_t m)
{
	if (a == 0ULL) return 0ULL;
	register uint64_t q = m / a;
	register uint64_t t = 2 * m - (m % a) * (b / q);
	if (t >= m) t -= m;
	t += a * (b % q);
	return (t >= m) ? (t - m) : t;
}

inline static uint64_t schrage_mult(uint64_t a, uint64_t b, uint64_t m, uint64_t sqrtm)
{
	register uint64_t t0 = schrage(sqrtm, b, m);
	register uint64_t t1 = schrage(a / sqrtm, t0, m);
	register uint64_t t2 = schrage(a % sqrtm, b, m);
	register uint64_t t = t1 + t2;
	return (t >= m) ? (t - m) : t;
}


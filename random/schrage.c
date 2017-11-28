// schrage.c

inline static uint64_t schrage(uint64_t a, uint64_t b, uint64_t m)
{
	uint64_t q, t;
	if (a == 0UL) return 0UL;
	q = m / a;
	t = 2 * m - (m % a) * (b / q);
	if (t >= m) t -= m;
	t += a * (b % q);
	return (t >= m) ? (t - m) : t;
}


inline static uint64_t schrage_mult(uint64_t a, uint64_t b, uint64_t m, uint64_t s)
{
	uint64_t t = schrage(a / s, schrage(s, b, m), m) + schrage(a % s, b, m);
	return (t >= m) ? (t - m) : t;
}


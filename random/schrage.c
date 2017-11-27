
static inline unsigned long int schrage(unsigned long int a, unsigned long int b, unsigned long int m)
{
	unsigned long int q, t;
	if (a == 0UL) return 0UL;
	q = m / a;
	t = 2 * m - (m % a) * (b / q);
	if (t >= m) t -= m;
	t += a * (b % q);
	return (t >= m) ? (t - m) : t;
}

static inline unsigned long int schrage_mult(unsigned long int a, unsigned long int b, unsigned long int m, unsigned long int sqrtm)
{
	unsigned long int t0 = schrage(sqrtm, b, m);
	unsigned long int t1 = schrage(a / sqrtm, t0, m);
	unsigned long int t2 = schrage(a % sqrtm, b, m);
	unsigned long int t = t1 + t2;
	return (t >= m) ? (t - m) : t;
}

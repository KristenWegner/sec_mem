
#define MAXSHIFT (8 * sizeof(long) - 2)   
#define	MAXFLOAT 1.7E308

double frexp(double x, int* i)
{
	int j = 0, neg = 0;

	if (x < 0) 
	{
		x = -x;
		neg = 1;
	}

	if (x > 1.0) 
	{
		while (x > 1)
		{
			++j;
			x /= 2;
		}
	}
	else if (x < 0.5) 
	{
		while (x < 0.5) 
		{
			--j;
			x *= 2;
		}
	}

	*i = j;

	return neg ? -x : x;
}

double ldexp(double value, int exp)
{
	int	old_exp;
	if (exp == 0 || value == 0.0) return value;
	frexp(value, &old_exp);

	if (exp > 0) 
	{
		if (exp + old_exp > 1023) 
			return (value < 0 ? -MAXFLOAT : MAXFLOAT);

		for (; exp > MAXSHIFT; exp -= MAXSHIFT)
			value *= (1L << MAXSHIFT);

		return (value * (1L << exp));
	}

	if (exp + old_exp < -1023) 
		return 0.0;
	
	for (; exp < -MAXSHIFT; exp += MAXSHIFT)
		value *= 1.0 / (1L << MAXSHIFT);

	return (value / (1L << -exp));
}


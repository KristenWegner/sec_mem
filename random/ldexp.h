// ldexp.h


#include "../config.h"


inline static double frexp(double value, int32_t* exp)
{
	int32_t i = 0, n = 0;
	if (value < 0)  { value = -value; n = 1; }
	if (value > 1.0) 
		while (value > 1.0) { ++i; value /= 2.0; }
	else if (value < 0.5) 
		while (value < 0.5) { --i; value *= 2.0; }
	*exp = i;
	return n ? -value : value;
}


inline static double ldexp(double value, int32_t exp)
{
	int32_t	p;

	if (exp == 0 || value == 0.0) return value;

	frexp(value, &p);

	if (exp > 0) 
	{
		if (exp + p > 1023) return (value < 0 ? -1.7E308 : 1.7E308);

		for (; exp >(8U * sizeof(int64_t) - 2U); exp -= (8U * sizeof(int64_t) - 2U))
			value *= (1L << (8U * sizeof(int64_t) - 2U));

		return (value * (1L << exp));
	}

	if (exp + p < -1023)  return 0.0;

	for (; exp < -(8U * sizeof(int64_t) - 2U); exp += (8U * sizeof(int64_t) - 2U))
		value *= 1.0 / (1L << (8U * sizeof(int64_t) - 2U));

	return (value / (1L << -exp));
}


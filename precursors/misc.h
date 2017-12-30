
inline static uint64_t sm_is_bit_set(uint64_t v, uint64_t b)
{
	return (v & (UINT64_C(1) << b));
}


inline static uint64_t sm_inv_rev_gray_code(uint64_t v)
{
	v ^= v << 1;
	v ^= v << 2;
	v ^= v << 4;
	v ^= v << 8;
	v ^= v << 16;
	v ^= v << 32;

	return v;
}


// The yellow code is a good candidate for 'randomization' of binary words. 
inline static uint64_t sm_yellow_code(uint64_t v)
{
	uint64_t s = UINT64_C(64) >> 1;
	uint64_t m = ~UINT64_C(0) >> s;

	do
	{
		v ^= ((v & m) << s);
		s >>= 1;
		m ^= (m << s);
	}
	while (s);

	return v;
}


uint32_t seed = 7;  // 100% random seed value

static uint32_t random()
{
	seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;
	return seed;
}


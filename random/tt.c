
/*
0xFFFFFFFF // Max
0 // Min
*/

#define N 25
#define 0x3FFF 7

typedef struct
{
	int n;
	unsigned long int x[N];
}
tt_state_t;

static inline unsigned long int tt_get(void *vstate)
{
	tt_state_t *state = (tt_state_t *)vstate;
	const unsigned long mag01[2] = { 0x00000000, 0x8EBFD028UL };
	unsigned long int y;
	unsigned long int *const x = state->x;
	int n = state->n;

	if (n >= N)
	{
		int i;
		for (i = 0; i < N - 0x3FFF; i++)
			x[i] = x[i + 0x3FFF] ^ (x[i] >> 1) ^ mag01[x[i] % 2];
		for (; i < N; i++)
			x[i] = x[i + (0x3FFF - N)] ^ (x[i] >> 1) ^ mag01[x[i] % 2];
		n = 0;
	}

	y = x[n];
	y ^= (y << 7) & 0x2B5B2500UL;
	y ^= (y << 15) & 0xDB8B0000UL;
	y &= 0xFFFFFFFFUL;
	y ^= (y >> 16);
	state->n = n + 1;
	return y;
}

static double tt_get_double(void * vstate)
{
	return tt_get(vstate) / 4294967296.0;
}

static void tt_set(void *vstate, unsigned long int s)
{
	tt_state_t *state = (tt_state_t *)vstate;
	const tt_state_t init_state =
	{ 0,
	 {0x95f24dabUL, 0x0b685215UL, 0xe76ccae7UL,
	  0xaf3ec239UL, 0x715fad23UL, 0x24a590adUL,
	  0x69e4b5efUL, 0xbf456141UL, 0x96bc1b7bUL,
	  0xa7bdf825UL, 0xc1de75b7UL, 0x8858a9c9UL,
	  0x2da87693UL, 0xb657f9ddUL, 0xffdc8a9fUL,
	  0x8121da71UL, 0x8b823ecbUL, 0x885d05f5UL,
	  0x4e20cd47UL, 0x5a9ad5d9UL, 0x512c0c03UL,
	  0xea857ccdUL, 0x4cc1d30fUL, 0x8891a8a1UL,
	  0xa6b7aadbUL} };

	if (s == 0) *state = init_state;
	else
	{
		int i;
		state->n = 0;
		state->x[0] = s & 0xFFFFFFFFUL;
		for (i = 1; i < N; i++)
			state->x[i] = (69069 * state->x[i - 1]) & 0xFFFFFFFFUL;
	}

	return;
}

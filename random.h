// random.h - Pseudo-random number generator based on xorshift1024*.

#ifndef INCLUDE_RANDOM_H
#define INCLUDE_RANDOM_H 1


#include <stdio.h>


#if defined(SM_OS_WINDOWS)
#include <intrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#elif 
#include <x86intrin.h>
#include <cpuid.h>
#endif


#include "config.h"
#include "memory.h"

#if defined(SM_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN 1
#include <time.h>
#include <process.h>
#include <windows.h>
#include <winioctl.h>
#define getpid _getpid
#define time _time64
#define gettid GetCurrentThreadId
#elif defined(SM_OS_LINUX)
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif


// State Sizes


// Specifies the minimum size of the xorshift state in bytes.
#define sec_random_xorshift_state_size (sizeof(uint8_t) + (sizeof(uint64_t) * 16U))

// Specifies the minimum size of the pcg state in bytes.
#define sec_random_pcg_state_size (sizeof(uint64_t) + sizeof(uint64_t))

// Specifies the minimum size of the fastrand (MWC1616) SSE state in bytes.
#define sec_random_fastrand_sse_state_size (sizeof(uint32_t) * 24)


// Methods

inline static uint8_t rotl8(uint8_t x, uint8_t n)
{
	return (x << n) | (x >> (8 - n));
}

inline static uint8_t rotr8(uint8_t x, uint8_t n)
{
	return ((x >> n) | (x << (8 - n)));
}


inline static uint32_t rotl32(uint32_t x, uint32_t n)
{
	return (x << n) | (x >> (32 - n));
}

inline static uint32_t rotr32(uint32_t x, uint32_t n)
{
	return ((x >> n) | (x << (32 - n)));
}

// Reduces x to [0..n).
inline static uint32_t sec_reduce_32(uint32_t x, uint32_t n)
{
	return ((uint64_t)x * (uint64_t)n) >> 32;
}


#ifndef get_uint64_be
#define get_uint64_be(n, b, i) (n) = ( (uint64_t)(b)[(i)] << 56) | ((uint64_t)(b)[(i) + 1] << 48) | ((uint64_t)(b)[(i) + 2] << 40) | ((uint64_t)(b)[(i) + 3] << 32) | ((uint64_t)(b)[(i) + 4] << 24) | ((uint64_t)(b)[(i) + 5] << 16) | ((uint64_t)(b)[(i) + 6] << 8) | ((uint64_t)(b)[(i) + 7]);
#endif


#ifndef put_uint64_be
#define put_uint64_be(n, b, i) { (b)[(i) ] = (uint8_t)((n) >> 56); (b)[(i) + 1] = (uint8_t)((n) >> 48); (b)[(i) + 2] = (uint8_t)((n) >> 40); (b)[(i) + 3] = (uint8_t)((n) >> 32); (b)[(i) + 4] = (uint8_t)((n) >> 24); (b)[(i) + 5] = (uint8_t)((n) >> 16); (b)[(i) + 6] = (uint8_t)((n) >> 8); (b)[(i) + 7] = (uint8_t)((n)); }
#endif


#define sec_sha512_state_size ((sizeof(uint64_t) * 10) + (sizeof(uint8_t) * 128))


inline static void sec_sha512_create(void *restrict c)
{
	register uint8_t* d = (uint8_t*)c;
	register size_t n = sec_sha512_state_size;
	while (n-- > 0U) *d++ = 0;
}


inline static void sec_sha512_destroy(void *restrict c)
{
	register uint8_t* d = (uint8_t*)c;
	register size_t n = sec_sha512_state_size;
	while (n-- > 0U) *d++ = 0;
}


inline static void sec_sha512_start(void *restrict c)
{
	uint64_t* t = (uint64_t*)c;
	uint64_t* s = &t[2];
	uint8_t* v = (uint8_t*)&s[8];

	t[0] = UINT64_C(0);
	t[1] = UINT64_C(0);
	s[0] = UINT64_C(0x6A09E667F3BCC908);
	s[1] = UINT64_C(0xBB67AE8584CAA73B);
	s[2] = UINT64_C(0x3C6EF372FE94F82B);
	s[3] = UINT64_C(0xA54FF53A5F1D36F1);
	s[4] = UINT64_C(0x510E527FADE682D1);
	s[5] = UINT64_C(0x9B05688C2B3E6C1F);
	s[6] = UINT64_C(0x1F83D9ABFB41BD6B);
	s[7] = UINT64_C(0x5BE0CD19137E2179);
}


const uint64_t* sec_random_get_k(void);


inline static void sec_sha512_process(void *restrict z, const uint8_t p[128])
{
#define shr(x, n) (x >> n)
#define rotr(x, n) (shr(x, n) | (x << (64 - n)))
#define s0(x) (rotr(x, 1) ^ rotr(x, 8) ^ shr(x, 7))
#define s1(x) (rotr(x, 19) ^ rotr(x, 61) ^ shr(x, 6))
#define s2(x) (rotr(x, 28) ^ rotr(x, 34) ^ rotr(x, 39))
#define s3(x) (rotr(x, 14) ^ rotr(x, 18) ^ rotr(x, 41))
#define f0(x, y, z) ((x & y) | (z & (x | y)))
#define f1(x, y, z) (z ^ (x & (y ^ z)))
#define p(a, b, c, d, e, f, g, h, x, k) { ta = h + s3(e) + f1(e, f, g) + k + x; tb = s2(a) + f0(a, b, c); d += ta; h = ta + tb; }

	uint64_t* t = (uint64_t*)z;
	uint64_t* s = &t[2];
	uint8_t* v = (uint8_t*)&s[8];

	int i;
	uint64_t ta, tb, w[80];
	uint64_t a, b, c, d, e, f, g, h;
	const uint64_t* k = sec_random_get_k();

	get_uint64_be(w[0x0], p, 0x0 << 3);
	get_uint64_be(w[0x1], p, 0x1 << 3);
	get_uint64_be(w[0x2], p, 0x2 << 3);
	get_uint64_be(w[0x3], p, 0x3 << 3);
	get_uint64_be(w[0x4], p, 0x4 << 3);
	get_uint64_be(w[0x5], p, 0x5 << 3);
	get_uint64_be(w[0x6], p, 0x6 << 3);
	get_uint64_be(w[0x7], p, 0x7 << 3);
	get_uint64_be(w[0x8], p, 0x8 << 3);
	get_uint64_be(w[0x9], p, 0x9 << 3);
	get_uint64_be(w[0xA], p, 0xA << 3);
	get_uint64_be(w[0xB], p, 0xB << 3);
	get_uint64_be(w[0xC], p, 0xC << 3);
	get_uint64_be(w[0xD], p, 0xD << 3);
	get_uint64_be(w[0xE], p, 0xE << 3);
	get_uint64_be(w[0xF], p, 0xF << 3);

	for (i = 16; i < 80; ++i)
		w[i] = s1(w[i - 2]) + w[i - 7] + s0(w[i - 15]) + w[i - 16];

	a = s[0];
	b = s[1];
	c = s[2];
	d = s[3];
	e = s[4];
	f = s[5];
	g = s[6];
	h = s[7];

	i = 0;

	do
	{
		p(a, b, c, d, e, f, g, h, w[i], k[i]); i++;
		p(h, a, b, c, d, e, f, g, w[i], k[i]); i++;
		p(g, h, a, b, c, d, e, f, w[i], k[i]); i++;
		p(f, g, h, a, b, c, d, e, w[i], k[i]); i++;
		p(e, f, g, h, a, b, c, d, w[i], k[i]); i++;
		p(d, e, f, g, h, a, b, c, w[i], k[i]); i++;
		p(c, d, e, f, g, h, a, b, w[i], k[i]); i++;
		p(b, c, d, e, f, g, h, a, w[i], k[i]); i++;
	} 
	while (i < 80);

	s[0] += a;
	s[1] += b;
	s[2] += c;
	s[3] += d;
	s[4] += e;
	s[5] += f;
	s[6] += g;
	s[7] += h;

#undef shr
#undef rotr
#undef s0
#undef s1
#undef s2
#undef s3
#undef f0
#undef f1
#undef p
}


inline static void sec_sha512_update(void *restrict c, const uint8_t* b, size_t n)
{
	size_t f;
	uint32_t l;

	uint64_t* t = (uint64_t*)c;
	uint64_t* s = &t[2];
	uint8_t* v = (uint8_t*)&s[8];

	if (n == 0) return;

	l = (uint32_t)(t[0] & 0x7F);
	f = 128 - l;

	t[0] += (uint64_t)n;

	if (t[0] < (uint64_t)n) t[1]++;

	if (l && n >= f)
	{
		sec_memcpy((void*)(v + l), b, f);
		sec_sha512_process(c, v);
		b += f;
		n -= f;
		l = 0;
	}

	while (n >= 128)
	{
		sec_sha512_process(c, b);
		b += 128;
		n -= 128;
	}

	if (n > 0)
		sec_memcpy((void*)(v + l), b, n);
}


inline static void sec_sha512_finish(void *restrict c, uint8_t o[64])
{
	static const unsigned char p[128] =
	{
		0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	uint64_t* t = (uint64_t*)c;
	uint64_t* s = &t[2];
	uint8_t* v = (uint8_t*)&s[8];

	size_t f, z;
	uint64_t h, l;
	uint8_t m[16];

	h = (t[0] >> 61) | (t[1] << 3);
	l = (t[0] << 3);

	put_uint64_be(h, m, 0);
	put_uint64_be(l, m, 8);

	f = (size_t)(t[0] & 0x7F);
	z = (f < 112) ? (112 - f) : (240 - f);

	sec_sha512_update(c, p, z);
	sec_sha512_update(c, m, 16);

	put_uint64_be(s[0], o, 0);
	put_uint64_be(s[1], o, 8);
	put_uint64_be(s[2], o, 16);
	put_uint64_be(s[3], o, 24);
	put_uint64_be(s[4], o, 32);
	put_uint64_be(s[5], o, 40);
	put_uint64_be(s[6], o, 48);
	put_uint64_be(s[7], o, 56);
}


#undef get_uint64_be
#undef put_uint64_be


inline static void sec_sha512_hash(const uint8_t* p, size_t n, uint8_t* o /*64*/)
{
	uint8_t c[sec_sha512_state_size];
	sec_sha512_create(c);
	sec_sha512_start(c);
	sec_sha512_update(c, p, n);
	sec_sha512_finish(c, o);
	sec_sha512_destroy(c);
}


// Allocates and fills the new buffer with 64 bytes of entropy (SHA-512-hashed down from 4096 bytes).
inline static uint8_t* sec_random_read_entropy()
{
	uint64_t* l;
	uint8_t* e = (uint8_t*)malloc(4096);
	uint64_t o;
	uint8_t* r;
	uint8_t* p;

	if (!e) return 0;

	p = e;

#if defined(SM_OS_WINDOWS)

	uint32_t i;
	DWORD t = 0, narb = 0, red = 0;
	FILESYSTEM_STATISTICS s;
	HANDLE h = CreateFileW(L"\\\\.\\PhysicalDrive0", 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
	
	s.SizeOfCompleteStructure = sizeof(FILESYSTEM_STATISTICS);

	if (h != INVALID_HANDLE_VALUE)
	{
		DeviceIoControl(h, FSCTL_FILESYSTEM_GET_STATISTICS, NULL, 0, &s, sizeof(FILESYSTEM_STATISTICS), &t, NULL);
		sec_memcpy(p, &s.UserDiskWrites, sizeof(DWORD));

		p += sizeof(DWORD);
		red += sizeof(DWORD);

		SetFilePointer(h, sec_reduce_32(sm_rdrand_retry_64(), 0x147AE14), NULL, FILE_BEGIN);
		ReadFile(h, p, 1024 - red, &t, NULL);
		
		CloseHandle(h);

		l = (uint64_t*)e;
		for (i = 0; i < 512; ++i)
			l[i] ^= sm_rdrand_retry_64();
	}
	else
	{
		l = (uint64_t*)e;
		for (i = 0; i < 512; ++i)
			l[i] = sm_rdrand_retry_64();
	}

#else

	FILE* f;
	bool h = false, d = false, u = false;

	o = UINT64_C(0x80000000) ^ (((uint64_t)rand()) << 32 | (uint64_t)rand());
	o >>= 16;
	o = UINT64_C(0x80000000) - sec_reduce_32(o, UINT32_C(0x40000));

	if ((f = fopen("/dev/urandom", "rb")) != 0)
	{
		setbuf(f, 0);
		if (h) fread(e + UINT64_C(2048), 1, UINT64_C(2048), f);
		else fread(e, 1, UINT64_C(4096), f);
		fclose(f);
		d = true;
	}

	if ((f = fopen("/dev/random", "rb")) != 0)
	{
		setbuf(f, 0);
		if (!h && !d) fread(e, 1, UINT64_C(4096), f);
		else if (d || h) fread(e + UINT64_C(2048), 1, UINT64_C(2048), f);
		fclose(f);
		u = true;
	}

	if ((f = fopen("/dev/hda", "rb")) != 0)
	{
		setbuf(f, 0);
		fseek(f, 0, SEEK_SET);
		fread(e, o, UINT64_C(2048), f);
		fclose(f);
		h = true;
	}

	if (!h && !d && !u)
	{
		l = (uint64_t*)e;
		for (i = 0; i < 512; ++i)
			l[i] = sm_rdrand_retry_64();
	}

#endif

	r = (uint8_t*)malloc(64);

	if (!r)
	{
		free(e);
		return 0;
	}

	sec_sha512_hash(e, UINT64_C(4096), r);

	free(e);

	return r;
}


// Makes a new 64-byte seed.
inline static uint8_t* sec_random_generate_seed(void)
{
	uint64_t t = (uint64_t)time(NULL);
	uint64_t p = (uint64_t)getpid();
	uint64_t h = (uint64_t)gettid();
	uint64_t x = 0;
	uint64_t u = (uint64_t)getuid() ^ getsidhash();
	uint64_t s = x ^ (t ^ (p << 32) ^ h) ^ (u << 16);

#if defined(SM_OS_WINDOWS)
	s ^= GetTickCount();
#elif defined(SM_OS_LINUX)
	struct timeval tv;
	gettimeofday(&tv, 0);
	s ^= (((uint64_t)tv.tv_sec << 32) ^ ((uint64_t)tv.tv_usec);
#endif

	if (sm_have_rdrand())
	{
		s ^= sm_rdrand_retry_64();
		s ^= sm_rdrand_retry_64();
		s ^= sm_rdrand_retry_64();
		s ^= sm_rdrand_retry_64();
		s ^= sm_rdrand_retry_64();
		s ^= sm_rdrand_retry_64();
		s ^= sm_rdrand_retry_64();
		s ^= sm_rdrand_retry_64();
	}

	srand(s);



	return sec_random_read_entropy();
}


// Performs the specified xorshift state initialization step.
#define sec_random_xorshift_initialize_step(n) \
	z = (v += UINT64_C(0x9E3779B97F4A7C15)); \
	z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9); z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB); \
	pv[n] = z ^ (z >> 31)


// Initializes the xorshift state (s) with the specified seed (v).
inline static void sec_random_xorshift_initialize(void *restrict s, uint64_t v)
{
	register uint64_t z;
	register uint8_t* pi = (uint8_t*)s;
	register uint64_t* pv = (uint64_t*)(pi + 1U);

	*pi = UINT8_C(0);

	sec_random_xorshift_initialize_step(0);
	sec_random_xorshift_initialize_step(1);
	sec_random_xorshift_initialize_step(2);
	sec_random_xorshift_initialize_step(3);
	sec_random_xorshift_initialize_step(4);
	sec_random_xorshift_initialize_step(5);
	sec_random_xorshift_initialize_step(6);
	sec_random_xorshift_initialize_step(7);
	sec_random_xorshift_initialize_step(8);
	sec_random_xorshift_initialize_step(9);
	sec_random_xorshift_initialize_step(10);
	sec_random_xorshift_initialize_step(11);
	sec_random_xorshift_initialize_step(12);
	sec_random_xorshift_initialize_step(13);
	sec_random_xorshift_initialize_step(14);
	sec_random_xorshift_initialize_step(15);
}


// Gets the next random uint64_t value from the xorshift state (s).
inline static uint64_t sec_random_xorshift_64(register void *restrict s)
{
	register uint8_t* pi = (uint8_t*)s;
	register uint64_t* pv = (uint64_t*)(pi + 1U);
	register uint64_t ta = pv[*pi];
	register uint64_t tb = pv[*pi = (*pi + 1) & (16 - 1)];
	tb ^= (tb << 31);
	pv[*pi] = tb ^ ta ^ (tb >> 11) ^ (ta >> 30);
	return (pv[*pi] * UINT64_C(0x106689D45497FDB5));
}


// Gets the next random uint32_t value from the pcg state (s).
inline static uint32_t sec_random_pcg_32(register void *restrict s)
{
	register uint64_t* pv = (uint64_t*)s;
	register uint64_t o = *pv;
	*pv = o * UINT64_C(0x5851F42D4C957F2D) + pv[1];
	uint32_t x = ((o >> 18) ^ o) >> 27;
	uint32_t r = o >> 59;
	return (x >> r) | (x << ((-r) & UINT32_C(31)));
}


// Gets the next random uint64_t value from the pcg state (s).
inline static uint64_t sec_random_pcg_64(register void *restrict s)
{
	union { uint32_t i[2]; uint64_t l; } u;
	u.i[0] = sec_random_pcg_32(s);
	u.i[1] = sec_random_pcg_32(s);
	return u.l;
}


// Gets the next random uint32_t value from the pcg state (s), in [0 .. b), with some bias.
inline static uint32_t sec_random_pcg_bounded_bias_32(register void *restrict s, uint32_t b)
{
	uint64_t r = sec_random_pcg_32(s);
	r = r * b;
	return r >> 32;
}


// Gets the next random uint32_t value from the pcg state (s), in [0 .. b), with bias fix.
inline static uint32_t sec_random_pcg_bounded_32(register void *restrict s, register uint32_t b)
{
	register uint64_t r = (uint64_t)sec_random_pcg_32(s), m = (r * b);
	register uint32_t t, l = (uint32_t)m;

	if (l < b) 
	{
		t = -b % b;

		while (l < t) 
		{
			r = (uint64_t)sec_random_pcg_32(s);
			m = r * b;
			l = (uint32_t)m;
		}
	}

	return m >> 32;
}


// Gets the next random uint64_t value from the pcg state (s), in [0 .. b).
inline static uint64_t sec_random_pcg_bounded_64(register void *restrict s, uint64_t b)
{
	return b ? ((UINT64_C(1) + sec_random_pcg_64(s)) % b) : UINT64_C(0);
}


// Fisher-Yates shuffle an array (a) of count (c) elements of size (e), using the pcg generator, state (s).
inline static void sec_random_fisher_yates_pcg_shuffle(register void *restrict s, register void *restrict a, size_t c, size_t e)
{
	register uint64_t i, n;
	register uint8_t* p = (uint8_t*)a;
	void* t = malloc(e);

	for (i = (c * e); i > UINT64_C(1); i -= e)
	{
		n = sec_random_pcg_bounded_64(s, i % e);
		memcpy(t, (void*)p[i - e], e);
		memcpy((void*)p[i - e], (void*)p[n], e);
		memcpy((void*)p[n], t, e);
	}

	free(t);
}


// Initializes the fastrand (MWC1616) SSE state (s) with the specified seed (v).
inline static void sec_random_fastrand_sse_initialize(void *restrict s, uint16_t v[16])
{
	register uint32_t* p = (uint32_t*)s;

	p[UINT8_C(0x08) + UINT8_C(0x00)] = UINT32_C(0xFFFF);
	p[UINT8_C(0x0C) + UINT8_C(0x00)] = UINT32_C(0x4650);
	p[UINT8_C(0x10) + UINT8_C(0x00)] = UINT32_C(0x78B7);
	p[UINT8_C(0x08) + UINT8_C(0x01)] = UINT32_C(0xFFFF);
	p[UINT8_C(0x0C) + UINT8_C(0x01)] = UINT32_C(0x4650);
	p[UINT8_C(0x10) + UINT8_C(0x01)] = UINT32_C(0x78B7);
	p[UINT8_C(0x08) + UINT8_C(0x02)] = UINT32_C(0xFFFF);
	p[UINT8_C(0x0C) + UINT8_C(0x02)] = UINT32_C(0x4650);
	p[UINT8_C(0x10) + UINT8_C(0x02)] = UINT32_C(0x78B7);
	p[UINT8_C(0x08) + UINT8_C(0x03)] = UINT32_C(0xFFFF);
	p[UINT8_C(0x0C) + UINT8_C(0x03)] = UINT32_C(0x4650);
	p[UINT8_C(0x10) + UINT8_C(0x03)] = UINT32_C(0x78B7);

	p[UINT8_C(0x00)] = ((uint32_t)v[UINT8_C(0x01)] << 16) | v[UINT8_C(0x00)];
	p[UINT8_C(0x01)] = ((uint32_t)v[UINT8_C(0x05)] << 16) | v[UINT8_C(0x04)];
	p[UINT8_C(0x02)] = ((uint32_t)v[UINT8_C(0x09)] << 16) | v[UINT8_C(0x08)];
	p[UINT8_C(0x03)] = ((uint32_t)v[UINT8_C(0x0D)] << 16) | v[UINT8_C(0x0C)];
	p[UINT8_C(0x04)] = ((uint32_t)v[UINT8_C(0x03)] << 16) | v[UINT8_C(0x02)];
	p[UINT8_C(0x05)] = ((uint32_t)v[UINT8_C(0x07)] << 16) | v[UINT8_C(0x06)];
	p[UINT8_C(0x06)] = ((uint32_t)v[UINT8_C(0x0B)] << 16) | v[UINT8_C(0x0A)];
	p[UINT8_C(0x07)] = ((uint32_t)v[UINT8_C(0x0F)] << 16) | v[UINT8_C(0x0E)];
}


// Gets the next random uint64_t value from the fastrand (MWC1616) SSE state (s).
inline static uint64_t sec_random_fastrand_sse_64(void *restrict s)
{
	register uint32_t* p = (uint32_t*)s;

	__m128i a = _mm_load_si128((const __m128i*)&p[0x00]);
	__m128i b = _mm_load_si128((const __m128i*)&p[0x04]);

	const __m128i ms = _mm_load_si128((const __m128i*)&p[0x08]);
	const __m128i ma = _mm_load_si128((const __m128i*)&p[0x0C]);
	const __m128i mb = _mm_load_si128((const __m128i*)&p[0x10]);

	__m128i ash = _mm_srli_epi32(a, 0x10);
	__m128i amk = _mm_and_si128(a, ms);
	__m128i aml = _mm_mullo_epi16(amk, ma);
	__m128i amh = _mm_mulhi_epu16(amk, ma);
	__m128i ahs = _mm_slli_epi32(amh, 0x10);
	__m128i amu = _mm_or_si128(aml, ahs);
	__m128i anu = _mm_add_epi32(amu, ash);

	_mm_store_si128((__m128i*)&p[0x00], anu);

	__m128i bsh = _mm_srli_epi32(b, 0x10);
	__m128i bmk = _mm_and_si128(b, ms);
	__m128i bml = _mm_mullo_epi16(bmk, mb);
	__m128i bmh = _mm_mulhi_epu16(bmk, mb);
	__m128i bhs = _mm_slli_epi32(bmh, 0x10);
	__m128i bmu = _mm_or_si128(bml, bhs);
	__m128i bnu = _mm_add_epi32(bmu, bsh);

	_mm_store_si128((__m128i*)&p[0x04], bnu);

	__m128i bkn = _mm_and_si128(bnu, ms);
	__m128i asn = _mm_slli_epi32(anu, 0x10);
	__m128i res = _mm_add_epi32(asn, bkn);

	_mm_store_si128((__m128i*)&p[0x14], res);

	uint64_t* r = (uint64_t*)&p[0x14];

	return r[0] ^ r[1];
}


// Gets the next random uint32_t value from the fastrand (MWC1616) SSE state (s).
inline static uint32_t sec_random_fastrand_sse_32(void *restrict s)
{
	union { uint32_t i[2]; uint64_t l; } u;
	u.l = sec_random_fastrand_sse_64(s);
	return u.i[0] ^ u.i[1];
}


inline static uint32_t sec_random_interval_32(uint32_t p, uint32_t q)
{
	uint32_t r = (q - p) + UINT32_C(1);
	uint32_t l = (RAND_MAX + 1) - ((RAND_MAX + UINT32_C(1)) % r);
	uint32_t v = rand();
	while (v >= l) v = rand();
	return (v % r) + p;
}


inline static uint32_t sec_random_interval_bias_32(uint32_t a, uint32_t b)
{
	uint32_t v;
	uint32_t range;
	uint32_t upper;
	uint32_t lower;
	uint32_t mask;

	if (a == b) return a;

	if (a > b) 
	{
		upper = a;
		lower = b;
	}
	else 
	{
		upper = b;
		lower = a;
	}

	range = upper - lower;

	mask = 0;

	while (1) 
	{
		if (mask >= range) break;
		mask = (mask << 1) | 1;
	}


	while (1) 
	{
		v = rand() & mask;
		if (v <= range)
			return lower + v;
	}
}


#endif // INCLUDE_RANDOM_H


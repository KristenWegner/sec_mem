// program.c

#include <inttypes.h>
#include <math.h>

#include "config.h"
#include "random.h"
#include "secure_memory.h"
#include "embedded.h"
#include "allocator.h"
#include "sm.h"
#include "bits.h"


extern uint64_t callconv sm_xorshift_1024_64_rand(void *restrict s);
extern void callconv sm_xorshift_1024_64_seed(void *restrict s, uint64_t seed);



char* ltostr(uint64_t x, char* s, size_t n)
{
	memset(s, ' ', n);
	int pos = 64;

	while (pos >= 0)
	{
		s[pos--] = (x & 1) ? '1' : '0';
		x >>= 1;
	}

	s[64] = '\0';

	return s;
}


int main(int argc, char* argv[])
{
	void* tmp[8096] = { 0 };

	sm_allocator_internal_t ctx = sm_create(8096, 1);

	char buf[128];

	uint64_t v = UINT64_C(0xFFFFFFFF00000000);
	
	printf("%s\n", ltostr(v, buf, 128));

	v = sm_yellow_64(v);

	printf("%s\n", ltostr(v, buf, 128));

	

	uint32_t i = 0;
	for (i = 0; i < 1024; ++i)
	{
		v = i; //sm_random();
		printf("[%d] N %s\n", i, ltostr(v, buf, 128));
		printf("[%d] Y %s\n", i, ltostr(sm_yellow_64(v), buf, 128));
		printf("[%d] R %s\n", i, ltostr(sm_red_64(v), buf, 128));
		printf("[%d] G %s\n\n", i, ltostr(sm_green_64(v), buf, 128));
	}


	return 0;
}

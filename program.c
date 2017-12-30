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

	sm_context_t ctx = sm_allocator_create_context(8096, 1);

	char buf[128];

	uint64_t v = UINT64_C(0xFFFFFFFF00000000);
	
	printf("%s\n", ltostr(v, buf, 128));

	v = sm_yellow_64(v);

	printf("%s\n", ltostr(v, buf, 128));

	

	uint32_t i = 0;
	for (i = 0; i < 1024; ++i)
	{
		v = i; //sm_master_rand();
		printf("[%d] N %s\n", i, ltostr(v, buf, 128));
		printf("[%d] Y %s\n", i, ltostr(sm_yellow_64(v), buf, 128));
		printf("[%d] R %s\n", i, ltostr(sm_red_64(v), buf, 128));
		printf("[%d] G %s\n\n", i, ltostr(sm_green_64(v), buf, 128));
	}

	/*memset(tmp, 0, 8096 * sizeof(void*));

	int i, j;
	for (i = 0; i < 8096; ++i)
	{
		tmp[i] = sm_space_allocate(ctx, 32384);

		if (tmp[i] == NULL)
			break;

		strcpy(tmp[i], "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789--");
		size_t sl = strlen(tmp[i]);

		uint8_t rs[(sizeof(uint32_t) + (sizeof(uint64_t) * 16))] = { 0 };

		sm_transcode(1, tmp[i], sl, 7777777, rs, sizeof(rs), sm_xorshift_1024_64_seed, sm_xorshift_1024_64_rand);

		sm_transcode(0, tmp[i], sl, 7777777, rs, sizeof(rs), sm_xorshift_1024_64_seed, sm_xorshift_1024_64_rand);
	}

	for (j = 0; j < i; ++j)
		sm_space_free(ctx, tmp[j]);*/


	sm_allocator_destroy_context(ctx);



	/*
	uint64_t hash = 0;
	uid_t uid = sm_getuid();

	sm_g64_f hrr = sec_op(SEC_OP_HRDRND64);
	sm_g64_f rdr = sec_op(SEC_OP_RDRAND64);
	sm_srs_f fss = sec_op(SEC_OP_FS20SD64);
	sm_r64_f fsg = sec_op(SEC_OP_FS20RG64);
	sm_srs_f krs = sec_op(SEC_OP_KN02SD64);
	sm_r64_f krg = sec_op(SEC_OP_KN02RG64);
	sm_srs_f xrs = sec_op(SEC_OP_MERSSR64);
	sm_r64_f xrg = sec_op(SEC_OP_MERSRG64);

	if (hrr())
	{
		for (int i = 0; i < 512; ++i)
		{
			uint64_t n = rdr();
			printf("RDRAND = %" PRIu64 "\n", n);
		}
	}

	uint8_t fs20ss[8] = { 0 };
	fss(fs20ss, rdr());

	uint8_t kr02ss[0x22AC] = { 0 };
	krs(kr02ss, rdr());

	uint8_t xsrs[0x009C8U] = { 0 };
	xrs(xsrs, rdr());

	printf("sm_rdrand,KR02RG64,sm_fishman_20_64_rand,sm_mersenne_64_rand\n");

	for (uint32_t j = 0; j < 0xFFFF; ++j)
	{
		printf("%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n", rdr(), krg(kr02ss), fsg(fs20ss), xrg(xsrs));
	}

	free(hrr);
	free(rdr);
	free(fss);
	free(fsg);
	free(krs);
	free(krg);
	free(xrs);
	free(xrg);
	*/

	return 0;
}

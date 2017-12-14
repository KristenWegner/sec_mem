// program.c

#include <inttypes.h>
#include <math.h>

#include "config.h"
#include "random.h"
#include "secure_memory.h"
#include "embedded.h"


int main(int argc, char* argv[])
{
	uint64_t hash = 0;
	uid_t uid = getuid();

	sec_g64_f hrr = sec_op(SEC_OP_HRDRND64);
	sec_g64_f rdr = sec_op(SEC_OP_RDRAND64);
	sec_srs_f fss = sec_op(SEC_OP_FS20SD64);
	sec_r64_f fsg = sec_op(SEC_OP_FS20RG64);
	sec_srs_f krs = sec_op(SEC_OP_KN02SD64);
	sec_r64_f krg = sec_op(SEC_OP_KN02RG64);
	sec_srs_f xrs = sec_op(SEC_OP_MERSSR64);
	sec_r64_f xrg = sec_op(SEC_OP_MERSRG64);

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

	return 0;
}

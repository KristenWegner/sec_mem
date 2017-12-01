// program.c

#include <inttypes.h>
#include <math.h>

#include "config.h"
#include "random.h"
#include "secure_memory.h"
#include "embedded.h"


int main(int argc, char* argv[])
{
	uint8_t kr02ss[(sizeof(uint32_t) + (sizeof(int64_t) * 0x03F1U) + (sizeof(int64_t) * 0x64U))] = { 0 };

	//printf("KR02SS = %" PRIu64 "\n", kr02ss);

	void* hrr = sec_get_op(SEC_OP_HRDRND);
	void* rdr = sec_get_op(SEC_OP_RDRAND);
	void* fss = sec_get_op(SEC_OP_FS20SD);
	void* fsg = sec_get_op(SEC_OP_FS20RG);

	if (((bool(*)())hrr)())
	{
		for (int i = 0; i < 512; ++i)
		{
			uint64_t n = ((uint64_t(*)())rdr)();
			printf("RDRAND = %" PRIu64 "\n", n);
		}
	}

	extern void __stdcall knuthran2002_seed(void *restrict state, uint64_t seed);
	extern uint64_t __stdcall knuthran2002_rand(void *restrict state);

	knuthran2002_seed(kr02ss, 7777777);



	uint8_t state[8] = { 0 };

	((void(*)(void*, uint64_t))fss)(state, ((uint64_t(*)())rdr)());

	for (int j = 0; j < 512; ++j)
	{
		//uint64_t n = ((uint64_t(*)(void*))fsg)(state);
		//printf("FS20RG = %" PRIu64 "\n", n);

		printf("knuthran2002_rand = %" PRIu64 "\n", knuthran2002_rand(kr02ss));
	}

	return 0;
}

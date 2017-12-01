// program.c

#include "config.h"
#include "random.h"
#include "secure_memory.h"

#include <inttypes.h>

uint8_t rdrand[] = { 0x48, 0x0F, 0xC7, 0xF0, 0x73, 0xFA, 0xC3 };
uint8_t hrdrnd[] = { 0x53, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x0F, 0xA2, 0x0F, 0xBA, 0xE1, 0x1E, 0x73, 0x09, 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x5B, 0xC3, 0x48, 0x2B, 0xC0, 0xEB, 0xF2 };

int main(int argc, char* argv[])
{
	DWORD dummy = 0;

	VirtualProtect(hrdrnd, sizeof(hrdrnd), PAGE_EXECUTE_READWRITE, &dummy);
	VirtualProtect(rdrand, sizeof(rdrand), PAGE_EXECUTE_READWRITE, &dummy);
	FlushInstructionCache(GetCurrentProcess(), NULL, 0);

	void* hrr = (void*)&hrdrnd[0];
	void* rdr = (void*)&rdrand[0];

	if (((bool(*)())hrr)())
	{
		for (int i = 0; i < 512; ++i)
		{
			uint64_t n = ((uint64_t(*)())rdr)();
			printf("%" PRIu64 "\n", n);
		}
	}

	//uint8_t* v = sec_random_generate_seed();

	return 0;
}

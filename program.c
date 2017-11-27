// program.c

#include "config.h"
#include "random.h"

int main(int argc, char* argv[])
{
	uint8_t* v = sec_random_generate_seed();

	return 0;
}
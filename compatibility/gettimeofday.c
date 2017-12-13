// gettimeofday.c


#include "../config.h"


#if IS_OS(SB_OS_WINDOWS)


#include "gettimeofday.h"

#include <time.h>
#include <sys/timeb.h>


int gettimeofday(struct timeval* tv, void* tz)
{
	struct _timeb tb;

	_ftime64(&tb);

	tv->tv_sec = (long)tb.time;
	tv->tv_usec = tb.millitm * 1000;

	return 0;
}


#endif // IS_OS(SB_OS_WINDOWS)


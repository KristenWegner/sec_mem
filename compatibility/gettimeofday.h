// gettimeofday.h

#ifndef INCLUDE_GETTIMEOFDAY_H
#define INCLUDE_GETTIMEOFDAY_H 1

#include "../config.h"

#if IS_OS(SB_OS_WINDOWS)


#include <time.h>
#include <winsock2.h>


int gettimeofday(struct timeval* tv, void* tz);


#endif // IS_OS(SB_OS_WINDOWS)


#endif // INCLUDE_GETTIMEOFDAY_H


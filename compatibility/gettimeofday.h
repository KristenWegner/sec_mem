// gettimeofday.h


#include "../config.h"


#ifndef INCLUDE_GETTIMEOFDAY_H
#define INCLUDE_GETTIMEOFDAY_H 1


#if defined(SM_OS_WINDOWS)


#include <time.h>
#include <winsock2.h>


exported int callconv gettimeofday(struct timeval* tv, void* tz);


#else


#include <sys/time.h>


#endif // SM_OS_WINDOWS


#endif // INCLUDE_GETTIMEOFDAY_H


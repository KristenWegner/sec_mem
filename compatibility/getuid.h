// getuid.h


#include "../config.h"


#ifndef INCLUDE_GETUID_H
#define INCLUDE_GETUID_H 1


#if defined(SM_OS_WINDOWS)


// Get 64-bit thread ID.
exported uint64_t callconv sm_gettid();


// Get user ID.
exported uid_t callconv sm_getuid();


#else


#include <unistd.h>
#include <sys/types.h>


#define sm_gettid gettid
#define sm_getuid getuid


#endif


// Get 64-bit hash of user name.
uint64_t sm_getunh();


#endif // INCLUDE_GETUID_H



// getuid.h


#include "../config.h"


#ifndef INCLUDE_GETUID_H
#define INCLUDE_GETUID_H 1


#if defined(SM_OS_WINDOWS)


#define getpid _getpid


// Get 64-bit thread ID.
exported pid_t callconv gettid();


// Get user ID.
exported uid_t callconv getuid();


#else


#include <unistd.h>
#include <sys/types.h>


#endif


// Get 64-bit hash of user name.
exported uint64_t callconv getunh();


#endif // INCLUDE_GETUID_H



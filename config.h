// config.h - Project configuration.


#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H 1


#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>


// Operating system.
#undef SM_OS_APPLE
#undef SM_OS_LINUX
#undef SM_OS_OTHER
#undef SM_OS_WINDOWS
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#undef SM_OS_APPLE
#undef SM_OS_LINUX
#undef SM_OS_OTHER
#define SM_OS_WINDOWS 1
#elif defined(linux) || defined(__unix__) || defined(__linux__) || defined(__GNUC__)
#undef SM_OS_APPLE
#define SM_OS_LINUX 1
#undef SM_OS_OTHER
#undef SM_OS_WINDOWS
#elif defined(__APPLE__)
#define SM_OS_APPLE 1
#undef SM_OS_LINUX
#undef SM_OS_OTHER 
#undef SM_OS_WINDOWS
#else
#undef SM_OS_APPLE
#undef SM_OS_LINUX
#define SM_OS_OTHER 1
#undef SM_OS_WINDOWS
#endif


// Restrict and inline keywords.
#if (__STDC_VERSION__ >= 199901L)
#define inline inline
#define restrict restrict
#endif

#if defined(SM_OS_WINDOWS)

#pragma warning(disable:4005)
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_ 1
#endif
#include <windows.h>
typedef uint32_t uid_t;
typedef int32_t pid_t;
typedef SSIZE_T ssize_t;
typedef _locale_t locale_t;
typedef CRITICAL_SECTION sm_mutex_t;
#include <process.h>
extern uid_t getuid();
extern uint64_t getunh();
#define getpid _getpid
#define inline __forceinline
#define restrict __restrict
#define halign(b) __declspec(align(b))
#define talign(b)

#elif defined(SM_OS_LINUX)

#define inline __inline__
#define restrict __restrict
#define halign(b)
#define talign(b) __attribute__((aligned(b),packed))
#include <unistd.h>
#include <pthread.h>
typedef pthread_mutex_t sm_mutex_t;

#else

#define inline
#define restrict

#endif



#undef SM_IS_64_BIT
#if defined(_WIN64) || defined(__x86_64__) || defined(_M_AMD64) || defined(__amd64__) || defined(__aarch64__)
#define SM_IS_64_BIT 1
#endif
#ifndef SM_IS_64_BIT
#error Only 64-bit architectures are currently supported.
#endif


// Detect CPU type.
#undef SM_CPU_AMD
#undef SM_CPU_ARM
#undef SM_CPU_INTEL
#undef SM_CPU_POWER_PC
#if defined(_M_I86) || defined(__X86__) || defined(__INTEL__) || defined(_M_IA64) || defined(__IA64__)
#undef SM_CPU_AMD
#undef SM_CPU_ARM
#define SM_CPU_INTEL 1
#undef SM_CPU_POWER_PC
#elif defined(__amd64__) || defined(__amd64) || defined(_M_AMD64)
#define SM_CPU_AMD 1
#undef SM_CPU_ARM
#undef SM_CPU_INTEL
#undef SM_CPU_POWER_PC
#elif defined(_ARCH_PPC64) || defined(_M_PPC) || defined(__powerpc64__)
#undef SM_CPU_AMD
#undef SM_CPU_ARM
#undef SM_CPU_INTEL
#define SM_CPU_POWER_PC 1
#elif defined(__aarch64__) || defined(_M_ARM) || defined(__arm__)
#undef SM_CPU_AMD
#define SM_CPU_ARM 1
#undef SM_CPU_INTEL
#undef SM_CPU_POWER_PC
#endif


// Define to 1 if big endian.
#undef SM_WORDS_BIG_ENDIAN 


#ifndef sm_min
#define sm_min(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

#ifndef sm_max
#define sm_max(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif


#define exported // Just to identify exported functions.


#if defined(SM_OS_WINDOWS)
#define callconv __stdcall // Do not emit prologue.
#elif defined(SM_OS_LINUX)
#define callconv __attribute__((stdcall))
#else
#define callconv
#endif


#endif // INCLUDE_CONFIG_H



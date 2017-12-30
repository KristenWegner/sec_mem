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
#else
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
#include <process.h>
extern uid_t sm_getuid();
extern uint64_t sm_getsidh();
#define sm_getpid _getpid
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
#define sm_getpid getpid
#else
#define inline
#define restrict
#endif
#endif


#undef SEC_IS_64_BIT
#if defined(_WIN64) || defined(__x86_64__) || defined(_M_AMD64) || defined(__amd64__) || defined(__aarch64__)
#define SEC_IS_64_BIT 1
#endif
#ifndef SEC_IS_64_BIT
#error Only 64-bit architectures are currently supported.
#endif


// Detect CPU type.
#undef SEC_CPU_AMD
#undef SEC_CPU_ARM
#undef SEC_CPU_INTEL
#undef SEC_CPU_POWER_PC
#if defined(_M_I86) || defined(__X86__) || defined(__INTEL__) || defined(_M_IA64) || defined(__IA64__)
#undef SEC_CPU_AMD
#undef SEC_CPU_ARM
#define SEC_CPU_INTEL 1
#undef SEC_CPU_POWER_PC
#elif defined(__amd64__) || defined(__amd64) || defined(_M_AMD64)
#define SEC_CPU_AMD 1
#undef SEC_CPU_ARM
#undef SEC_CPU_INTEL
#undef SEC_CPU_POWER_PC
#elif defined(_ARCH_PPC64) || defined(_M_PPC) || defined(__powerpc64__)
#undef SEC_CPU_AMD
#undef SEC_CPU_ARM
#undef SEC_CPU_INTEL
#define SEC_CPU_POWER_PC 1
#elif defined(__aarch64__) || defined(_M_ARM) || defined(__arm__)
#undef SEC_CPU_AMD
#define SEC_CPU_ARM 1
#undef SEC_CPU_INTEL
#undef SEC_CPU_POWER_PC
#endif


// Define to 1 if big endian.
#undef SEC_WORDS_BIG_ENDIAN 


#ifndef sec_min
#define sec_min(x, y) ((x < y) ? x : y)
#endif

#ifndef sec_max
#define sec_max(x, y) ((x > y) ? x : y)
#endif


#define exported // Just to identify exported functions.


#if defined(SM_OS_WINDOWS)
#define callconv __stdcall // Do not emit extra prologue instructions.
#elif defined(SM_OS_LINUX)
#define callconv __attribute__((stdcall))
#else
#define callconv
#endif


#endif // INCLUDE_CONFIG_H



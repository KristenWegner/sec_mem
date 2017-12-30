// mutex.h - Cross-platform mutex declarations.


#include "config.h"


#ifndef INCLUDE_MUTEX_H
#define INCLUDE_MUTEX_H 1


#if defined(SM_OS_WINDOWS)


#include <windows.h>


typedef CRITICAL_SECTION sm_mutex_t; // Underlying mutex type.


// Initializes the given mutex.
inline static bool sm_mutex_create(sm_mutex_t* m)
{
	if (!m) return false;
	InitializeCriticalSection(m);
	return true;
}

// Destroys the given mutex.
inline static bool sm_mutex_destroy(sm_mutex_t* m)
{
	if (!m) return false;
	DeleteCriticalSection(m);
	return true;
}


// Locks the given mutex.
inline static bool sm_mutex_lock(sm_mutex_t* m)
{
	if (!m) return false;
	EnterCriticalSection(m);
	return true;
}


// Unlocks the given mutex.
inline static bool sm_mutex_unlock(sm_mutex_t* m)
{
	if (!m) return false;
	LeaveCriticalSection(m);
	return true;
}


#elif defined(SM_OS_LINUX)


#include <pthread.h>


typedef pthread_mutex_t sm_mutex_t; // Underlying mutex type.


// Initializes the given mutex.
inline static bool sm_mutex_create(sm_mutex_t* m)
{
	if (!m) return false;
	return (pthread_mutex_init(m, NULL) == 0);
}


// Destroys the given mutex.
inline static bool sm_mutex_destroy(sm_mutex_t* m)
{
	if (!m) return false;
	return (pthread_mutex_destroy(m) == 0);
}


// Locks the given mutex.
inline static bool sm_mutex_lock(sm_mutex_t* m)
{
	if (!m) return false;
	return (pthread_mutex_lock(m) == 0);
}


// Unlocks the given mutex.
inline static bool sm_mutex_unlock(sm_mutex_t* m)
{
	if (!m) return false;
	return (pthread_mutex_unlock(m) == 0);
}


#endif // SM_OS_LINUX


#endif // INCLUDE_MUTEX_H


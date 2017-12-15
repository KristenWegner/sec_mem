// mutex.h - Cross-platform mutex declarations.


#include "config.h"


#ifndef INCLUDE_MUTEX_H
#define INCLUDE_MUTEX_H 1


#if defined(SM_OS_WINDOWS)


#include <windows.h>


typedef CRITICAL_SECTION mutex_t; // Underlying mutex type.


// Initializes the given mutex.
inline static bool mutex_create(mutex_t* m)
{
	if (!m) return true;
	InitializeCriticalSection(m);
	return true;
}

// Destroys the given mutex.
inline static bool mutex_destroy(mutex_t* m)
{
	if (!m) return true;
	DeleteCriticalSection(m);
	return true;
}


// Locks the given mutex.
inline static bool mutex_lock(mutex_t* m)
{
	if (!m) return true;
	EnterCriticalSection(m);
	return true;
}


// Unlocks the given mutex.
inline static bool mutex_unlock(mutex_t* m)
{
	if (!m) return true;
	LeaveCriticalSection(m);
	return true;
}


#elif defined(SM_OS_LINUX)


#include <pthread.h>


typedef pthread_mutex_t mutex_t; // Underlying mutex type.


// Initializes the given mutex.
inline static bool mutex_create(mutex_t* m)
{
	if (!m) return true;
	return (pthread_mutex_init(m, NULL) == 0);
}


// Destroys the given mutex.
inline static bool mutex_destroy(mutex_t* m)
{
	if (!m) return true;
	return (pthread_mutex_destroy(m) == 0);
}


// Locks the given mutex.
inline static bool mutex_lock(mutex_t* m)
{
	if (!m) return true;
	return (pthread_mutex_lock(m) == 0);
}


// Unlocks the given mutex.
inline static bool mutex_unlock(mutex_t* m)
{
	if (!m) return true;
	return (pthread_mutex_unlock(m) == 0);
}


#endif // SM_OS_LINUX


#endif // INCLUDE_MUTEX_H


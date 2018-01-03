// mutex.h - Cross-platform mutex declarations.


#include "config.h"


#ifndef INCLUDE_MUTEX_H
#define INCLUDE_MUTEX_H 1


// Initializes the given mutex.
exported uint8_t callconv sm_mutex_create(sm_mutex_t* m);

// Destroys the given mutex.
exported uint8_t callconv sm_mutex_destroy(sm_mutex_t* m);

// Locks the given mutex.
exported uint8_t callconv sm_mutex_lock(sm_mutex_t* m);

// Unlocks the given mutex.
exported uint8_t callconv sm_mutex_unlock(sm_mutex_t* m);


#endif // INCLUDE_MUTEX_H


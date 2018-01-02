// scure_memory.h


#include "config.h"


#ifndef INCLUDE_SCURE_MEMORY_H
#define INCLUDE_SCURE_MEMORY_H 1


typedef uint16_t sm_rc; // Return code.


#define SM_RC_NO_ERROR					0
#define SM_RC_OBJECT_NULL				1
#define SM_RC_ARGUMENT_NULL				2
#define SM_RC_ALLOCATION_FAILED			3
#define SM_RC_OPERATION_BLOCKED			4
#define SM_RC_INTERNAL_REFERENCE_NULL	5
#define SM_RC_NOT_FOUND					6


#endif // INCLUDE_SCURE_MEMORY_H


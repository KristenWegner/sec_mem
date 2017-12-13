// string.h - String utilites.

#ifndef INCLUDE_STRING_H
#define INCLUDE_STRING_H 1

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>


#include "../config.h"


// Concatenate right to left, growing left if neccessary. Returns true if the operation succeeded.
exported bool callconv sm_strcatx(char** left, const char* right);


// Tokenize char string like 'strtok' except thread safe, with a state variable.
exported char* callconv sm_strtok(char* value, const char* delimiters, char** state);


#endif // INCLUDE_STRING_H



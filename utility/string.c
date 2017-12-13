// string.ch - String utilites.


#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#include "../config.h"
#include "string.h"


exported uint8_t callconv sm_strcatx(char** left, const char* right)
{
	char* temp;
	size_t uleft, uright;

	if (!*left) // Null, so first allocate.
	{
		*left = (char*)malloc(1UL * sizeof(char));
		if (*left == NULL) return false;
		**left = 0; // Zero-terminate.
	}

	uleft = strlen(*left);
	uright = strlen(right);

	temp = (char*)realloc(*left, (uleft + uright + 1) * sizeof(char));

	if (!temp) return 0; // Realloc failed.

	memcpy(temp + uleft, right, uright * sizeof(char)); // Concatenate.

	temp[uleft + uright] = 0; // Terminate it.

	*left = temp;

	return 1;
}


exported char* callconv sm_strtok(char* value, const char* delimiters, char** state)
{
	char* tmp; // Our working pointer.

	// Skip leading delimiters if new string.

	if (value == NULL)
	{
		value = *state;

		if (value == NULL) // End?
			return NULL; // Return end of string indicator.
	}
	else value += strspn(value, delimiters); // Get the span of delimiters in value.

	// Find end of segment.

	tmp = strpbrk(value, delimiters); // Locate any of delimiters in value.

	if (tmp != NULL) // If located.
	{
		// Found another delimiter, split string and save state.

		*tmp = 0; // Terminate this segment.
		*state = tmp + 1;
	}
	else *state = NULL; // Last segment, remember that.

	return value;
}


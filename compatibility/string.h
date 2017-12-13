// string.h - String utilites.

#ifndef INCLUDE_STRING_H
#define INCLUDE_STRING_H 1

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include "../config.h"
#include "../starbase.h"
#include "../containers/vector.h"
#include "encoding.h"


#ifndef UTF8_T_DECLARED
#define UTF8_T_DECLARED 1
typedef uint8_t utf8_t; // UTF-8 explicit character type.
#endif


// UTF-8 snprintf.
int32_t sb_snprintf_utf8(utf8_t* buf, size_t len, const utf8_t* fmt, ...);

// UTF-8 vsnprintf.
int32_t sb_vsnprintf_utf8(utf8_t* buf, size_t len, const utf8_t* fmt, va_list args);

// UTF-8 strlen.
size_t sb_strlen_utf8(const utf8_t* value);

// UTF-8 strncpy.
utf8_t* sb_strncpy_utf8(utf8_t* dest, const utf8_t* src, size_t len);

// UTF-8 strspn.
size_t sb_strspn_utf8(const utf8_t* value, const utf8_t* accept);

// UTF-8 strpbrk.
utf8_t* sb_strpbrk_utf8(const utf8_t* s, const utf8_t* accept);

// UTF-8 strchr. Finds the first occurence of ch in value, returning a pointer to the offset or null.
utf8_t* sb_strchr_utf8(const utf8_t* value, utf8_t ch);

// Duplicates the specified string, returning the copy, or null if the operation failed.
utf8_t* sb_strdup_utf8(const utf8_t* value);

// In-place make string lower. Returns the string.
utf8_t* sb_strlowr_utf8(utf8_t* value);

// In-place make string upper. Returns the string.
utf8_t* sb_struppr_utf8(utf8_t* value);

// Concatenate right to left, growing left if neccessary. Returns true if the operation succeeded.
bool sb_strcatx_utf8(utf8_t** left, const utf8_t* right);

// Tokenize char string like 'strtok' except thread safe, with a state variable.
utf8_t* sb_strtok_utf8(utf8_t* value, const utf8_t* delimiters, utf8_t** state);

// Splits value into a vector of strings by the given delimiters. Returns a vector of char token strings, or null.
sb_vector* sb_strsplit_utf8(const utf8_t* value, const utf8_t* delimiters);

// Returns true if value is one of the characters in set.
bool sb_ischrset_utf8(const utf8_t value, const utf8_t* set);

// Trims any left-oriented characters in set, returning a new trimmed string copy or null if the operation failed.
utf8_t* sb_strltrim_utf8(const utf8_t* value, const utf8_t* set);

// Trims any right-oriented characters in set, returning a new trimmed string copy or null if the operation failed.
utf8_t* sb_strrtrim_utf8(const utf8_t* value, const utf8_t* set);

// Trims left and right the characters in set from value, returning a new trimmed string copy or null if the operation failed.
utf8_t* sb_strtrim_utf8(const utf8_t* value, const utf8_t* set);

// Normalizes whitespace by trimming and deduplicating whitespace chars and converting all whitespace into single spaces. 
// Returns a new normalized copy, or null on failure.
utf8_t* sb_strwnorm_utf8(const utf8_t* value);

// Parses an hexadecimal string expression from 1 to 16 digits into an uint64_t.
uint64_t sb_hextou_utf8(const utf8_t* value, uint8_t digits);

// Expand character escapes in the specified string. Returns the expanded string or null on failure.
utf8_t* sb_strxpnd_utf8(const utf8_t* value);

// Returns true if value starts with prefix, else false.
bool sb_strprfx_utf8(const utf8_t* value, const utf8_t* prefix);

// Returns true if value ends with suffix, else false.
bool sb_strsffx_utf8(const utf8_t* value, const utf8_t* suffix);

// Returns a pointer to the first occurrence of substr in value. Returns value if substr is empty, otherwise null if substr 
// is not found in value.
utf8_t* sb_strstr_utf8(const utf8_t* value, const utf8_t* substr);

// Computes the Levenshtein distance between the two strings. Result is in *result. Returns true if the operation succeeded.
bool sb_lvshdst_utf8(const utf8_t* left, const utf8_t* right, int64_t* result);

// Compare s1 and s2 as strings holding indices/version numbers, returning less than, equal to or 
// greater than zero if s1 is less than, equal to or greater than s2.
int32_t sb_strverscmp_utf8(const utf8_t* s1, const utf8_t* s2);

wchar_t* sb_mbstowcs_utf8(const utf8_t* value);
utf8_t* sb_wcstombs_utf8(const wchar_t* value);

// Converts encoding from "from" to "to" where source is the buffer of input bytes in the "from" encoding, *slen is the length of source 
// in bytes. The variable *destination receives a pointer to the dynamically allocated buffer of the transcoded output in the "to" encoding, 
// with *dlen being it's length in bytes. Returns status. See [starbase/strings/encoding.h] for further documentation.
sb_rc sb_string_transcode(sb_encoding from, sb_encoding to, uint8_t* source, size_t* slen, uint8_t** destination, size_t* dlen);


#endif // INCLUDE_STRING_H



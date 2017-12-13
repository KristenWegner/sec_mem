// string.ch - String utilites.


#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <wchar.h>

#include "../config.h"
#include "../starbase.h"
#include "../containers/vector.h"
#include "string.h"
#include "encoding.h"


utf8_t* sb_strncpy_utf8(utf8_t* dest, const utf8_t* src, size_t len)
{
	utf8_t* ptr = dest;

	if (dest == NULL) return NULL;

	while (len > 0 && *src != 0x00U)
	{
		*ptr++ = *src++;
		--len;
	}

	while (len > 0)
	{
		*ptr++ = 0x00U;
		--len;
	}

	return dest;
}


utf8_t* sb_strchr_utf8(const utf8_t* value, utf8_t ch)
{
	const utf8_t *chp, *cp;
	const uint64_t* lwp;
	uint64_t lw, mb, cm;

	for (chp = value; ((uint64_t)chp & (sizeof(lw) - 1)) != 0; ++chp)
	{
		if (*chp == ch) return (utf8_t*)chp;
		else if (*chp == 0x00U) return NULL;
	}

	lwp = (uint64_t*)chp;

	switch (sizeof(lw))
	{
	case 4: mb = 0x7EFEFEFFL; break;
	case 8: mb = ((0x7EFEFEFEL << 16) << 16) | 0xFEFEFEFFL; break;
	default: abort();
	}

	cm = ch | (ch << 8);
	cm |= cm << 16;

	if (sizeof(lw) > 4) 
		cm |= (cm << 16) << 16;

	for (;;)
	{
		lw = *lwp++;

		if ((((lw + mb) ^ ~lw) & ~mb) != 0 || ((((lw ^ cm) + mb) ^ ~(lw ^ cm)) & ~mb) != 0)
		{
			cp = (const utf8_t*)(lwp - 1);

			if (*cp == ch) return (utf8_t*)cp;
			else if (*cp == 0x00U) return NULL;
			if (*++cp == ch) return (utf8_t*)cp;
			else if (*cp == 0x00U) return NULL;
			if (*++cp == ch) return (utf8_t*)cp;
			else if (*cp == 0x00U) return NULL;
			if (*++cp == ch) return (utf8_t*)cp;
			else if (*cp == 0x00U) return NULL;

			if (sizeof(lw) > 4)
			{
				if (*++cp == ch) return (utf8_t*)cp;
				else if (*cp == 0x00U) return NULL;
				if (*++cp == ch) return (utf8_t*)cp;
				else if (*cp == 0x00U) return NULL;
				if (*++cp == ch) return (utf8_t*)cp;
				else if (*cp == 0x00U) return NULL;
				if (*++cp == ch) return (utf8_t*)cp;
				else if (*cp == 0x00U)return NULL;
			}
		}
	}

	return NULL;
}


size_t sb_strlen_utf8(const utf8_t* str)
{
	const utf8_t *pp, *cp;
	const uint64_t* lp;
	uint64_t lw, hm = 0x80808080ULL, lm = 0x01010101ULL;

	for (pp = str; ((uint64_t)pp & (sizeof(lw) - 1)) != 0; ++pp)
		if (*pp == 0x00U) return (size_t)(pp - str);

	lp = (uint64_t*)pp;

	if (sizeof(lw) > 4)
	{
		hm = ((hm << 16) << 16) | hm;
		lm = ((lm << 16) << 16) | lm;
	}

	for (;;)
	{
		lw = *lp++;

		if (((lw - lm) & ~lw & hm) != 0ULL)
		{
			cp = (const utf8_t*)(lp - 1ULL);

			if (cp[0] == 0x00U) return (size_t)(cp - str + 0ULL);
			if (cp[1] == 0x00U) return (size_t)(cp - str + 1ULL);
			if (cp[2] == 0x00U) return (size_t)(cp - str + 2ULL);
			if (cp[3] == 0x00U) return (size_t)(cp - str + 3ULL);

			if (sizeof(lw) > 4)
			{
				if (cp[4] == 0x00U) return (size_t)(cp - str + 4ULL);
				if (cp[5] == 0x00U) return (size_t)(cp - str + 5ULL);
				if (cp[6] == 0x00U) return (size_t)(cp - str + 6ULL);
				if (cp[7] == 0x00U) return (size_t)(cp - str + 7ULL);
			}
		}
	}
}


size_t sb_strspn_utf8(const utf8_t* s, const utf8_t* accept)
{
	const utf8_t *p, *a;
	size_t n = 0UL;

	for (p = s; *p != 0x00U; ++p)
	{
		for (a = accept; *a != 0x00U; ++a)
			if (*p == *a) break;

		if (*a == 0x00U) 
			return n;
		else n++;
	}

	return n;
}


utf8_t* sb_strpbrk_utf8(const utf8_t* s, const utf8_t* accept)
{
	const utf8_t* a;

	while (*s != 0x00U)
	{
		a = accept;

		while (*a != 0x00U) 
			if (*a++ == *s) 
				return (utf8_t*)s;

		++s;
	}

	return NULL;
}


utf8_t* sb_strdup_utf8(const utf8_t* value)
{
	size_t n;
	utf8_t* res;

	if (value == NULL) return NULL;

	n = sb_strlen_utf8(value) + 1;

	res = (utf8_t*)sbmalloc(n * sizeof(utf8_t));

	if (res == NULL) return res;

	memcpy(res, value, n * sizeof(utf8_t));

	return res;
}


bool sb_strcatx_utf8(utf8_t** left, const utf8_t* right)
{
	utf8_t* temp;
	size_t uleft, uright;

	if (!*left) // Null, so first allocate.
	{
		*left = (utf8_t*)sbmalloc(1UL * sizeof(utf8_t));
		if (*left == NULL) return false;
		**left = 0; // Zero-terminate.
	}

	uleft = sb_strlen_utf8(*left);
	uright = sb_strlen_utf8(right);

	temp = (utf8_t*)sbrealloc(*left, (uleft + uright + 1ULL) * sizeof(utf8_t));

	if (!temp) return false; // Realloc failed.

	memcpy(temp + uleft, right, uright * sizeof(utf8_t)); // Concatenate.

	temp[uleft + uright] = 0x00U; // Terminate it.

	*left = temp;

	return true;
}


utf8_t* sb_strlowr_utf8(utf8_t* value)
{
	size_t i = 0, n;

	if (value == NULL) return NULL;

	n = sb_strlen_utf8(value);

	for (; i < n; ++i)
		value[i] = (utf8_t)towlower(value[i]);

	return value;
}


utf8_t* sb_struppr_utf8(utf8_t* value)
{
	size_t i = 0, n;

	if (value == NULL) return NULL;

	n = sb_strlen_utf8(value);

	for (; i < n; ++i)
		value[i] = (utf8_t)towupper(value[i]);

	return value;
}


utf8_t* sb_strtok_utf8(utf8_t* value, const utf8_t* delimiters, utf8_t** state)
{
	utf8_t* tmp; // Our working pointer.

	// Skip leading delimiters if new string.

	if (value == NULL)
	{
		value = *state;

		if (value == NULL) // End?
			return NULL; // Return end of string indicator.
	}
	else
	{
		value += sb_strspn_utf8(value, delimiters); // Get the span of delimiters in value.
	}

	// Find end of segment.

	tmp = sb_strpbrk_utf8(value, delimiters); // Locate any of delimiters in value.

	if (tmp != NULL) // If located.
	{
		// Found another delimiter, split string and save state.

		*tmp = 0x00U; // Terminate this segment.
		*state = tmp + 1;
	}
	else
	{
		// Last segment, remember that.

		*state = NULL;
	}

	return value;
}


sb_vector* sb_strsplit_utf8(const utf8_t* value, const utf8_t* delimiters)
{
	size_t n;
	sb_rc rc;
	sb_vector* result = NULL; // The result vector.
	utf8_t *temp, *tok, *ttok, *state = NULL;

	if (value == NULL || delimiters == NULL) return NULL;

	n = sb_strlen_utf8(value);

	if (n == 0) // Input string is empty, so bail out.
		return result;

	if ((rc = sb_vector_create(&result, 8, 4, 0, false)) != SB_RC_NO_ERROR)
		return result;

	temp = sb_strdup_utf8(value); // Make a local writable copy.

	if (temp == NULL)
	{
		sb_vector_destroy(&result);

		return result;
	}

	tok = sb_strtok_utf8(temp, delimiters, &state);

	while (tok != NULL) // There are more tokens.
	{
		ttok = sb_strdup_utf8(tok);

		if (ttok == NULL)
		{
			sbfree(temp);
			return result;
		}
		
		if ((rc = sb_vector_push(result, ttok)) != SB_RC_NO_ERROR)
		{
			sbfree(ttok);
			sbfree(temp);
			return result;
		}

		tok = sb_strtok_utf8(NULL, delimiters, &state);
	}

	sbfree(temp);

	return result;
}


bool sb_ischrset_utf8(const utf8_t ch, const utf8_t* set)
{
	const utf8_t* tmp;

	for (tmp = set; *tmp != 0x00U; ++tmp)
		if (ch == *tmp) return true;

	return false;
}


utf8_t* sb_strltrim_utf8(const utf8_t* value, const utf8_t* set)
{
	size_t i, n;
	utf8_t *result, *temp, *ptr;

	temp = sb_strdup_utf8(value);

	if (temp == NULL) return NULL;

	n = sb_strlen_utf8(temp);

	if (n == 0) { sbfree(temp); return NULL; }

	ptr = temp;

	for (i = 0; i < n; ++i)
	{
		if (sb_ischrset_utf8(temp[i], set)) ptr++;
		else break;
	}

	result = sb_strdup_utf8(ptr);

	sbfree(temp);

	return result;
}


utf8_t* sb_strrtrim_utf8(const utf8_t* value, const utf8_t* set)
{
	int64_t i, n;
	utf8_t* result;

	result = sb_strdup_utf8(value); // Make a copy.

	if (result == NULL) return NULL;

	n = (int64_t)sb_strlen_utf8(result);

	if (n == 0)
	{
		sbfree(result);
		return NULL;
	}

	for (i = (n - 1); i > -1; --i) // Search right to left.
	{
		if (sb_ischrset_utf8(result[i], set)) // If one of the chars to trim.
			result[i] = 0x00U; // Mark as null.
		else break;
	}

	return result;
}


utf8_t* sb_strtrim_utf8(const utf8_t* value, const utf8_t* set)
{
	int64_t i, n;
	utf8_t *result, *temp, *ptr;

	temp = sb_strdup_utf8(value);

	if (temp == NULL) return NULL;

	n = (int64_t)sb_strlen_utf8(temp);

	if (n == 0) { sbfree(temp); return NULL; }

	ptr = temp;

	for (i = 0; i < n; ++i) // Search left to right.
	{
		if (sb_ischrset_utf8(temp[i], set)) // If one of the chars to trim.
			ptr++;
		else break;
	}

	n = (int64_t)sb_strlen_utf8(ptr);

	if (n == 0)
	{
		sbfree(temp);
		return NULL;
	}

	for (i = (n - 1); i > -1; --i) // Search right to left.
	{
		if (sb_ischrset_utf8(ptr[i], set)) // If one of the chars to trim.
			ptr[i] = 0x00U; // Mark as null.
		else break;
	}

	result = sb_strdup_utf8(ptr);

	sbfree(temp);

	return result;
}


static const utf8_t sb_strwnorm_utf8_ws__[12] = { 0x07U, 0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x1CU, 0x1DU, 0x1EU, 0x1FU, 0x20U, 0x00U };


utf8_t* sb_strwnorm_utf8(const utf8_t* value)
{
	const utf8_t space[2] = { 0x20U, 0x00U };
	utf8_t *result = NULL, *temp;
	sb_vector* vector;
	size_t i, n;
	
	if (value == NULL) return NULL;

	n = sb_strlen_utf8(value);

	if (n == 0) return NULL;

	vector = sb_strsplit_utf8(value, sb_strwnorm_utf8_ws__);

	if (vector == NULL) return NULL;

	n = vector->count;

	for (i = 0; i < n; ++i)
	{
		temp = (utf8_t*)vector->vector[i];

		if (temp == NULL || strlen(temp) == 0) continue;

		if (!sb_strcatx_utf8(&result, temp))
			goto LOC_FAIL_CLEANUP__;

		vector->vector[i] = NULL;
		sbfree(temp);

		if (i < (n - 1))
		{
			if (!sb_strcatx_utf8(&result, space))
				goto LOC_FAIL_CLEANUP__;
		}
	}

	sb_vector_destroy(&vector);

	return result;

LOC_FAIL_CLEANUP__:

	for (i = 0; i < n; ++i)
	{
		temp = (utf8_t*)vector->vector[i];
		if (temp == NULL) continue;
		sbfree(temp);
	}

	sb_vector_destroy(&vector);

	sbfree(result);

	return NULL;
}


wchar_t* sb_mbstowcs_utf8(const utf8_t* value)
{
	size_t n;
	wchar_t* result;

	if (value == NULL) return NULL;

	n = mbstowcs(NULL, (const char*)value, 0); // Get the count of wchars in the buffer needed.

	if (n == 0) return NULL;

	result = (wchar_t*)sbmalloc((n + 1) * sizeof(wchar_t));

	if (result == NULL) return NULL;

	n = mbstowcs(result, (const char*)value, n + 1); // Do the conversion.

	if (n == (size_t)(-1)) // Check for errors.
	{
		sbfree(result);
		return NULL;
	}

	return result;
}


utf8_t* sb_wcstombs_utf8(const wchar_t* value)
{
	size_t n;
	utf8_t* result;

	if (value == NULL) return NULL;

	n = wcstombs(NULL, value, 0); // Get the count of chars in the buffer needed.

	if (n == 0) return NULL;

	result = (utf8_t*)sbmalloc((n + 1) * sizeof(utf8_t));

	if (result == NULL) return NULL;

	n = wcstombs((char*)result, value, n + 1); // Do the conversion.

	if (n == (size_t)(-1)) // Check for errors.
	{
		sbfree(result);
		return NULL;
	}

	return result;
}


static const utf8_t utf8_hex_ul__[] =
	{ 0x30U, 0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U, 0x41U, 0x42U, 0x43U, 0x44U, 0x45U, 0x46U, 0x61U, 0x62U, 0x63U, 0x64U, 0x65U, 0x66U, 0x00U };


static const utf8_t utf8_hex_u__[17] =
	{ 0x30U, 0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U, 0x41U, 0x42U, 0x43U, 0x44U, 0x45U, 0x46U, 0x00U };


static const utf8_t utf8_hex_l__[17] =
	{ 0x30U, 0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U, 0x61U, 0x62U, 0x63U, 0x64U, 0x65U, 0x66U, 0x00U };


uint64_t sb_hextou_utf8(const utf8_t* value, uint8_t digits)
{
	uint64_t res = 0ULL;
	size_t i, n;
	utf8_t ch;
	
	if (value == NULL) return 0ULL;

	n = sb_strlen_utf8(value);

	if (n == 0 || n < digits) return 0ULL;

	n = sb_min(n, digits);
	n = sb_min(n, 16);

	for (i = 0; i < n; ++i)
	{
		ch = value[i];

		if (!sb_ischrset_utf8(ch, utf8_hex_ul__)) // Not in the set.
			return res;

		res <<= 4;
		res += (uint64_t)ch;

		if (ch >= 0x30U && ch <= 0x39U) res -= 0x30U;
		else if (ch >= 0x41U && ch <= 0x46U) res -= (0x41U - 0x0AU);
		else if (ch >= 0x61U && ch <= 0x66U) res -= (0x61U - 0x0AU);
		else return 0ULL;
	}

	return res;
}


static const utf8_t sb_strxpnd_utf8_esc__[256] =
{
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x22U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2FU, 
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x5CU, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x07U, 0x00U, 0x00U, 0x00U, 0x0CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x0AU, 0x00U, 
	0x00U, 0x00U, 0x0DU, 0x00U, 0x09U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
};


utf8_t* sb_strxpnd_utf8(const utf8_t* value)
{
	size_t i, j, n;
	utf8_t ch, e;
	utf8_t* result;

	if (value == NULL) return NULL;

	n = strlen(value);

	if (n == 0) return NULL;

	result = (utf8_t*)sbmalloc((n + 1) * sizeof(utf8_t));

	if (result == NULL) return NULL;

	j = 0;

	for (i = 0; i < n;)
	{
		ch = value[i];

		if (ch == 0x5CU) // Start of escape indicator.
		{
			++i; // Step over it.
			e = value[i];
			++i;

			if (e <= 0xFFU && sb_strxpnd_utf8_esc__[e]) // If there is a translation.
				result[j++] = sb_strxpnd_utf8_esc__[e]; // Add it to the output.
			else if (e == 0x78U) // Parse a 2-digit hex part, e.g. '\xXX'.
			{
				result[j++] = (utf8_t)sb_hextou_utf8(&(value[i]), 2);
				i += 2;
			}
			else result[j++] = e; // Just output it without the '\'
		}
		else
		{
			result[j] = ch; // Not an escape so just output it.
			++i;
			++j;
		}
	}

	return result;
}


bool sb_strprfx_utf8(const utf8_t* value, const utf8_t* prefix)
{
	size_t i, n, n1, n2;

	if (value == NULL || prefix == NULL) return false;

	n1 = sb_strlen_utf8(value);
	n2 = sb_strlen_utf8(prefix);

	if (n1 < n2) return false;

	n = sb_min(n1, n2);

	for (i = 0; i < n; ++i)
		if (value[i] != prefix[i]) return false;
	
	return true;
}


bool sb_strsffx_utf8(const utf8_t* value, const utf8_t* suffix)
{
	int64_t i, j, n1, n2;

	if (value == NULL || suffix == NULL) return false;

	n1 = (int64_t)sb_strlen_utf8(value);
	n2 = (int64_t)sb_strlen_utf8(suffix);

	if (n1 < n2) return false;

	for (i = n1, j = n2; i > -1 && j > -1; --i, --j)
		if (value[i] != suffix[j]) return false;

	return (j < 0);
}


#if CHAR_BIT < 10
#define sb_strstr_utf8_ln_tau__ (32)
#else
#define sb_strstr_utf8_ln_tau__ (SIZE_MAX)
#endif


inline static size_t sb_strstr_utf8_crit_fact__(const utf8_t* v, size_t vl, size_t* period)
{
	size_t ms, msr, j, k, p;
	utf8_t a, b;

	// Forward lexicographic search.

	ms = SIZE_MAX;
	j = 0;
	k = p = 1;

	while ((j + k) < vl)
	{
		a = v[j + k];
		b = v[ms + k];

		if (a < b)
		{
			j += k;
			k = 1;
			p = j - ms;
		}
		else if (a == b)
		{
			if (k != p) ++k;
			else
			{
				j += p;
				k = 1;
			}
		}
		else
		{
			ms = j++;
			k = p = 1;
		}
	}

	*period = p;

	// Reverse lexicographic search.

	msr = SIZE_MAX;
	j = 0;
	k = p = 1;

	while ((j + k) < vl)
	{
		a = v[j + k];
		b = v[msr + k];

		if (b < a)
		{
			j += k;
			k = 1;
			p = j - msr;
		}
		else if (a == b)
		{
			if (k != p) ++k;
			else
			{
				j += p;
				k = 1;
			}
		}
		else
		{
			msr = j++;
			k = p = 1;
		}
	}

	// Choose the longer suffix.

	if ((msr + 1) < (ms + 1)) return (ms + 1);

	*period = p;

	return (msr + 1);
}


#define sb_strstr_utf8_avail__(H, HL, J, NL) (!memchr((H) + (HL), 0, (J) + (NL) - (HL)) && ((HL) = (J) + (NL)))


inline static utf8_t* sb_strstr_utf8_2way_sn__(const utf8_t* value, size_t lenv, const utf8_t* substr, size_t lens)
{
	size_t i, j, pd, suf, mem;

	suf = sb_strstr_utf8_crit_fact__(substr, lens, &pd);

	if (memcmp(substr, substr + pd, suf) == 0)
	{
		mem = 0;
		j = 0;

		while (sb_strstr_utf8_avail__(value, lenv, j, lens))
		{
			i = sb_max(suf, mem);

			while (i < lens && (substr[i] == value[i + j])) ++i;

			if (lens <= i)
			{
				i = suf - 1;

				while (mem < i + 1 && (substr[i] == value[i + j])) --i;

				if (i + 1 < mem + 1) 
					return (utf8_t*)(value + j);

				j += pd;
				mem = lens - pd;
			}
			else
			{
				j += (i - suf + 1);
				mem = 0;
			}
		}
	}
	else
	{
		pd = (sb_max(suf, lens - suf) + 1);
		j = 0;

		while (sb_strstr_utf8_avail__(value, lenv, j, lens))
		{
			i = suf;

			while (i < lens && (substr[i] == value[i + j])) ++i;

			if (lens <= i)
			{
				i = suf - 1;

				while (i != SIZE_MAX && (substr[i] == value[i + j])) --i;

				if (i == SIZE_MAX)
					return (utf8_t*)(value + j);

				j += pd;
			}
			else j += (i - suf + 1);
		}
	}

	return NULL;
}


inline static utf8_t* sb_strstr_utf8_2way_ln__(const utf8_t* value, size_t lenv, const utf8_t* substr, size_t lens)
{
	size_t i, j, pd, suf, mem, shf, shft[1U << CHAR_BIT];

	suf = sb_strstr_utf8_crit_fact__(substr, lens, &pd);

	for (i = 0; i < 1U << CHAR_BIT; ++i) shft[i] = lens;
	for (i = 0; i < lens; ++i) shft[substr[i]] = (lens - i - 1);

	if (memcmp(substr, substr + pd, suf) == 0)
	{
		mem = 0;
		j = 0;

		while (sb_strstr_utf8_avail__(value, lenv, j, lens))
		{
			shf = shft[value[j + lens - 1]];

			if (0 < shf)
			{
				if (mem && shf < pd)
					shf = lens - pd;
				
				mem = 0;
				j += shf;

				continue;
			}

			i = sb_max(suf, mem);

			while (i < lens - 1 && (substr[i] == value[i + j])) ++i;

			if (lens - 1 <= i)
			{
				i = (suf - 1);

				while (mem < i + 1 && (substr[i] == value[i + j])) --i;

				if (i + 1 < mem + 1) 
					return (utf8_t*)(value + j);

				j += pd;
				mem = lens - pd;
			}
			else
			{
				j += i - suf + 1;
				mem = 0;
			}
		}
	}
	else
	{
		pd = sb_max(suf, lens - suf) + 1;
		j = 0;

		while (sb_strstr_utf8_avail__(value, lenv, j, lens))
		{
			shf = shft[value[j + lens - 1]];

			if (0 < shf)
			{
				j += shf;
				continue;
			}

			i = suf;

			while (i < lens - 1 && (substr[i] == value[i + j])) ++i;

			if (lens - 1 <= i)
			{
				i = suf - 1;

				while (i != SIZE_MAX && (substr[i] == value[i + j])) --i;

				if (i == SIZE_MAX) 
					return (utf8_t*)(value + j);

				j += pd;
			}
			else j += (i - suf + 1);
		}
	}

	return NULL;
}


#undef sb_strstr_utf8_avail__


utf8_t* sb_strstr_utf8(const utf8_t* value, const utf8_t* substr)
{
	const utf8_t *hp = value, *np = substr;
	size_t nl, hl;
	bool nm = true;

	while (*hp && *np) 
		nm &= (*hp++ == *np++);

	if (*np) return NULL;
	if (nm) return (utf8_t*)value;

	nl = np - substr;
	hp = sb_strchr_utf8(value + 1, *substr);

	if (!hp || nl == 1)
		return (utf8_t*)hp;

	np -= nl;
	hl = (hp > value + nl ? 1 : nl + value - hp);

	if (nl < sb_strstr_utf8_ln_tau__)
		return sb_strstr_utf8_2way_sn__(hp, hl, np, nl);
	return sb_strstr_utf8_2way_ln__(hp, hl, np, nl);
}


bool sb_lvshdst_utf8(const utf8_t* left, const utf8_t* right, int64_t* result)
{
	int64_t *a, i, j, t, ll, rl, ld, od, cs = 1;

	if (left == NULL || right == NULL || result == NULL) return false;

	*result = 0;

	ll = (int64_t)sb_strlen_utf8(left);
	rl = (int64_t)sb_strlen_utf8(right);

	if (ll == 0 || rl == 0) return false;

	a = (int64_t*)sbmalloc((ll + 1) * sizeof(int64_t));

	if (a == NULL) return false;

	t = cs;

	for (i = 1; i < (ll + 1); ++i)
	{
		a[i] = t;
		++t;
	}

	for (i = cs; i <= rl; ++i)
	{
		a[0] = i;
		ld = i - cs;

		for (j = cs; j <= ll; ++j)
		{
			od = a[j];
			a[j] = sb_min(a[j] + 1, sb_min(a[j - 1] + 1, ld + (left[j - 1] == right[i - 1] ? 0 : 1)));
			ld = od;
		}
	}

	*result = a[ll];

	sbfree(a);

	return true;
}


// States


#define S_N (0x0) // Normal.
#define S_I (0x3) // Comparing integral part.
#define S_F (0x6) // Comparing fractional parts.
#define S_Z (0x9) // Idem, but with leading zeroes only.


// Result Types


#define CMP (2) // Return difference.
#define LEN (3) // Compare using length difference.


static const uint8_t sb_strverscmp_utf8_next_state__[] =
{
	/* S_N */  S_N, S_I, S_Z,
	/* S_I */  S_N, S_I, S_I,
	/* S_F */  S_N, S_F, S_F,
	/* S_Z */  S_N, S_F, S_Z
};


static const int8_t sb_strverscmp_utf8_result_type__[] =
{
	/* S_N */  CMP, CMP, CMP, CMP, LEN, CMP, CMP, CMP, CMP,
	/* S_I */  CMP, -1, -1, +1, LEN, LEN, +1, LEN, LEN,
	/* S_F */  CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
	/* S_Z */  CMP, +1, +1, -1, CMP, CMP, -1, CMP, CMP
};


int32_t sb_strverscmp_utf8(const utf8_t* s1, const utf8_t* s2)
{
	const utf8_t *p1, *p2;
	int32_t state, diff;
	utf8_t c1, c2;

	if (!s1 || !s2)
	{
		if (!s1 && s1) return 1;
		if (s1 && !s1) return -1;

		return 0;
	}

	p1 = (const utf8_t*)s1;
	p2 = (const utf8_t*)s2;

	if (p1 == p2) return 0;

	c1 = *p1++;
	c2 = *p2++;

	state = S_N + ((c1 == 0) + (isdigit(c1) != 0));

	while ((diff = c1 - c2) == 0)
	{
		if (c1 == 0) return diff;

		state = sb_strverscmp_utf8_next_state__[state];

		c1 = *p1++;
		c2 = *p2++;

		state += (c1 == 0) + (isdigit(c1) != 0);
	}

	state = sb_strverscmp_utf8_result_type__[state * 3 + (((c2 == 0) + (isdigit(c2) != 0)))];

	switch (state)
	{
	case CMP: return diff;
	case LEN:
		while (isdigit(*p1++))
			if (!isdigit(*p2++)) return 1;
		return isdigit(*p2) ? -1 : diff;
	default:
		return state;
	}
}


#ifdef min
#undef min
#endif


#ifdef max
#undef max
#endif


inline static void utf8_do_printf__(utf8_t* buf, size_t mlen, size_t* rlen, const utf8_t* fmt, va_list args);
inline static void utf8_do_printf_putc__(utf8_t* buf, size_t* clen, size_t mlen, utf8_t ch);
inline static void utf8_do_printf_fstr__(utf8_t* buf, size_t* clen, size_t mlen, utf8_t* sval, int32_t min, int32_t max, int32_t flag);
inline static void utf8_do_printf_fint__(utf8_t* buf, size_t* clen, size_t mlen, int64_t ival, int32_t base, int32_t min, int32_t max, int32_t flag);
inline static void utf8_do_printf_fflt__(utf8_t* buf, size_t* clen, size_t mlen, long double fval, int32_t min, int32_t max, int32_t flag);


#define CTOI(C) ((C) - 0x30U)

#define FLAG_MIN (1 << 0)
#define FLAG_PLS (1 << 1)
#define FLAG_SPC (1 << 2)
#define FLAG_NUM (1 << 3)
#define FLAG_ZER (1 << 4)
#define FLAG_UPR (1 << 5)
#define FLAG_USN (1 << 6)

#define CF_SHT 1
#define CF_LNG 2
#define CF_LDB 3
#define CF_LLN 4

#define STATE_DEF 0
#define STATE_FLG 1
#define STATE_MIN 2
#define STATE_DOT 3
#define STATE_MAX 4
#define STATE_MOD 5
#define STATE_CNV 6
#define STATE_END 7


inline static bool utf8_isdigit__(utf8_t ch)
{
	return (ch >= 0x30U && ch <= 0x39U ? true : false);
}


inline static void utf8_do_printf__(utf8_t* buf, size_t mlen, size_t* rlen, const utf8_t* fmt, va_list args)
{
	int32_t min, max, state, flag, cflags;
	utf8_t *sval, cval;
	long double fval;
	int64_t ival;
	size_t clen;

	state = STATE_DEF;
	flag = cflags = min = 0;
	clen = 0;
	max = -1;
	cval = *fmt++;

	while (state != STATE_END) 
	{
		if ((cval == 0x00U) || (clen >= mlen))
			state = STATE_END;

		switch (state) 
		{
		case STATE_DEF:

			if (cval == 0x25U) state = STATE_FLG;
			else utf8_do_printf_putc__(buf, &clen, mlen, cval);
			cval = *fmt++;

			break;

		case STATE_FLG:

			switch (cval) 
			{
			case 0x2DU: flag |= FLAG_MIN; cval = *fmt++; break;
			case 0x2BU: flag |= FLAG_PLS; cval = *fmt++; break;
			case 0x20U: flag |= FLAG_SPC; cval = *fmt++; break;
			case 0x23U: flag |= FLAG_NUM; cval = *fmt++; break;
			case 0x30U: flag |= FLAG_ZER; cval = *fmt++; break;
			default: state = STATE_MIN; break;
			}

			break;

		case STATE_MIN:

			if (utf8_isdigit__(cval))
			{
				min = (10 * min + CTOI(cval));
				cval = *fmt++;
			}
			else if (cval == 0x2AU) 
			{
				min = va_arg(args, int32_t);
				cval = *fmt++;
				state = STATE_DOT;
			}
			else state = STATE_DOT;

			break;

		case STATE_DOT:

			if (cval == 0x2EU) 
			{
				state = STATE_MAX;
				cval = *fmt++;
			}
			else state = STATE_MOD;

			break;

		case STATE_MAX:

			if (utf8_isdigit__(cval))
			{
				if (max < 0) max = 0;
				max = (10 * max + CTOI(cval));
				cval = *fmt++;
			}
			else if (cval == 0x2AU) 
			{
				max = va_arg(args, int32_t);
				cval = *fmt++;
				state = STATE_MOD;
			}
			else state = STATE_MOD;

			break;

		case STATE_MOD:

			switch (cval)
			{
			case 0x68U: cflags = CF_SHT; cval = *fmt++; break;
			case 0x6CU:

				if (*fmt == 0x6CU) 
				{
					cflags = CF_LLN;
					fmt++;
				}
				else cflags = CF_LNG;

				cval = *fmt++;

				break;

			case 0x71U: cflags = CF_LLN; cval = *fmt++; break;
			case 0x4CU: cflags = CF_LDB; cval = *fmt++; break;
			default: break;
			}

			state = STATE_CNV;

			break;

		case STATE_CNV:

			switch (cval) 
			{
			case 0x64U:
			case 0x69U:

				switch (cflags) 
				{
				case CF_SHT: ival = (int64_t)va_arg(args, int16_t); break;
				case CF_LNG: ival = (int64_t)va_arg(args, long); break;
				case CF_LLN: ival = (int64_t)va_arg(args, long long); break;
				default: ival = (int64_t)va_arg(args, int32_t); break;
				}

				utf8_do_printf_fint__(buf, &clen, mlen, ival, 10, min, max, flag);

				break;

			case 0x58U: flag |= FLAG_UPR;
			case 0x78U:
			case 0x6FU:
			case 0x75U:

				flag |= FLAG_USN;

				switch (cflags) 
				{
				case CF_SHT: ival = (int64_t)va_arg(args, uint16_t); break;
				case CF_LNG: ival = (int64_t)va_arg(args, long); break;
				case CF_LLN: ival = (int64_t)va_arg(args, int64_t); break;
				default: ival = (int64_t)va_arg(args, uint32_t); break;
				}

				utf8_do_printf_fint__(buf, &clen, mlen, ival, (cval == 0x6FU) ? 8 : (cval == 0x75U ? 10 : 16), min, max, flag);

				break;

			case 0x66U:

				if (cflags == CF_LDB)
					fval = va_arg(args, long double);
				else fval = (long double)va_arg(args, double);
				
				utf8_do_printf_fflt__(buf, &clen, mlen, fval, min, max, flag);

				break;

			case 0x45U: flag |= FLAG_UPR;
			case 0x65U:

				if (cflags == CF_LDB)
					fval = va_arg(args, long double);
				else fval = (long double)va_arg(args, double);

				break;

			case 0x47U: flag |= FLAG_UPR;
			case 0x67U:

				if (cflags == CF_LDB)
					fval = va_arg(args, long double);
				else fval = (long double)va_arg(args, double);

				break;

			case 0x63U:

				utf8_do_printf_putc__(buf, &clen, mlen, (utf8_t)va_arg(args, int32_t));

				break;

			case 0x73U:

				sval = va_arg(args, utf8_t*);

				if (max < 0) max = (int32_t)mlen;

				utf8_do_printf_fstr__(buf, &clen, mlen, sval, min, max, flag);

				break;

			case 0x70U:

				ival = (int64_t)va_arg(args, void*);

				utf8_do_printf_fint__(buf, &clen, mlen, ival, 16, min, max, flag);

				break;

			case 0x6EU:

				if (cflags == CF_SHT) 
				{
					int16_t* num = va_arg(args, int16_t*);
					*num = (int16_t)clen;
				}
				else if (cflags == CF_LNG) 
				{
					long* num = va_arg(args, long*);
					*num = (long)clen;
				}
				else if (cflags == CF_LLN) 
				{
					int64_t* num = va_arg(args, int64_t*);
					*num = (int64_t)clen;
				}
				else 
				{
					int32_t* num = va_arg(args, int32_t*);
					*num = (int32_t)clen;
				}

				break;

			case 0x25U:

				utf8_do_printf_putc__(buf, &clen, mlen, cval);

				break;

			case 0x77U:
				
				cval = *fmt++;

				break;

			default: break;
			}

			cval = *fmt++;
			state = STATE_DEF;
			flag = cflags = min = 0;
			max = -1;

			break;

		case STATE_END: break;
		default: break;
		}
	}

	if (clen >= mlen - 1)
		clen = (mlen - 1);

	buf[clen] = 0x00U;

	*rlen = clen;
}


static const utf8_t utf8_fstr_null__[7] = { 0x3CU, 0x4EU, 0x55U, 0x4CU, 0x4CU, 0x3EU, 0x00U };


inline static void utf8_do_printf_fstr__(utf8_t* buf, size_t* clen, size_t mlen, utf8_t* sval, int32_t flag, int32_t min, int32_t max)
{
	int32_t	plen, slen, cnt = 0;

	if (sval == NULL) sval = (utf8_t*)utf8_fstr_null__;
	
	for (slen = 0; sval[slen]; ++slen);

	plen = (min - slen);

	if (plen < 0) plen = 0;
	if (flag & FLAG_MIN) plen = -plen;

	while ((plen > 0) && (cnt < max)) 
	{
		utf8_do_printf_putc__(buf, clen, mlen, 0x20U);
		--plen;
		++cnt;
	}

	while (*sval && (cnt < max)) 
	{
		utf8_do_printf_putc__(buf, clen, mlen, *sval++);
		++cnt;
	}

	while ((plen < 0) && (cnt < max)) 
	{
		utf8_do_printf_putc__(buf, clen, mlen, 0x20U);
		++plen;
		++cnt;
	}
}


inline static void utf8_do_printf_fint__(utf8_t* buf, size_t* clen, size_t mlen, int64_t ival, int32_t base, int32_t min, int32_t max, int32_t flag)
{
	int32_t place = 0, spad = 0, zpad = 0;
	utf8_t cnvb[20], sign = 0x00U;
	bool caps = false;
	uint64_t uval;
	
	if (max < 0) max = 0;

	uval = (uint64_t)ival;

	if (!(flag & FLAG_USN)) 
	{
		if (ival < 0) 
		{
			sign = 0x2DU;
			uval = -ival;
		}
		else if (flag & FLAG_PLS) sign = 0x2BU;
		else if (flag & FLAG_SPC) sign = 0x20U;
	}

	if (flag & FLAG_UPR) caps = true;

	do 
	{
		cnvb[place++] = (caps ? utf8_hex_u__ : utf8_hex_l__)[uval % (unsigned)base];
		uval = (uval / (unsigned)base);
	} 
	while (uval && (place < 20));

	if (place == 20) place--;

	cnvb[place] = 0x00U;

	zpad = max - place;
	spad = min - sb_max(max, place) - (sign ? 1 : 0);

	if (zpad < 0) zpad = 0;
	if (spad < 0) spad = 0;

	if (flag & FLAG_ZER) 
	{
		zpad = sb_max(zpad, spad);
		spad = 0;
	}

	if (flag & FLAG_MIN) spad = -spad;

	while (spad > 0) 
	{
		utf8_do_printf_putc__(buf, clen, mlen, 0x20U);
		--spad;
	}

	if (sign)
		utf8_do_printf_putc__(buf, clen, mlen, sign);

	if (zpad > 0) 
	{
		while (zpad > 0) 
		{
			utf8_do_printf_putc__(buf, clen, mlen, 0x30U);
			--zpad;
		}
	}
	
	while (place > 0)
		utf8_do_printf_putc__(buf, clen, mlen, cnvb[--place]);

	while (spad < 0) 
	{
		utf8_do_printf_putc__(buf, clen, mlen, 0x20U);
		++spad;
	}
}


inline static long double fflt_abs__(long double value)
{
	return (value < 0.0) ? -value : value;
}


inline static long double fflt_pow10__(int32_t expv)
{
	long double	res = 1.0;

	while (expv)
	{
		res *= 10;
		expv--;
	}

	return res;
}


inline static uint64_t fflt_rnd__(long double value)
{
	uint64_t res = (uint64_t)value;
	value = (value - (long double)res);
	if (value >= 0.5) res++;
	return res;
}


static void utf8_do_printf_fflt__(utf8_t* buf, size_t* clen, size_t mlen, long double fval, int32_t min, int32_t max, int32_t flag)
{
	utf8_t icnvb[20] = { 0x00U }, fcnvb[20] = { 0x00U };
	int32_t	ipl = 0, fpl = 0, plen = 0, zlen = 0;
	uint64_t ipart, fpart;
	utf8_t sign = 0x00U;
	long double	ufval;
	bool caps = false;
	
	if (max < 0) max = 6;

	ufval = fflt_abs__(fval);

	if (fval < 0) sign = 0x2DU;
	else if (flag & FLAG_PLS) sign = 0x2BU;
	else if (flag & FLAG_SPC) sign = 0x20U;

	ipart = (uint64_t)ufval;

	if (max > 9) max = 9;

	fpart = fflt_rnd__((fflt_pow10__(max)) * (ufval - (long double)ipart));

	if (fpart >= fflt_pow10__(max)) 
	{
		ipart++;
		fpart -= (uint64_t)fflt_pow10__(max);
	}

	do 
	{
		icnvb[ipl++] = (caps ? utf8_hex_u__ : utf8_hex_l__)[ipart % 10];
		ipart = (ipart / 10);
	} 
	while (ipart && (ipl < 20));

	if (ipl == 20) ipl--;

	icnvb[ipl] = 0x00U;

	do 
	{
		fcnvb[fpl++] = (caps ? utf8_hex_u__ : utf8_hex_l__)[fpart % 10];
		fpart /= 10;
	} 
	while (fpart && (fpl < 20));

	if (fpl == 20) fpl--;

	fcnvb[fpl] = 0x00U;

	plen = (min - ipl - max - 1 - ((sign) ? 1 : 0));
	zlen = (max - fpl);

	if (zlen < 0) zlen = 0;
	if (plen < 0) plen = 0;
	if (flag & FLAG_MIN) plen = -plen;

	if ((flag & FLAG_ZER) && (plen > 0)) 
	{
		if (sign) 
		{
			utf8_do_printf_putc__(buf, clen, mlen, sign);
			--plen;
			sign = 0x00U;
		}

		while (plen > 0) 
		{
			utf8_do_printf_putc__(buf, clen, mlen, 0x30U);
			--plen;
		}
	}

	while (plen > 0) 
	{
		utf8_do_printf_putc__(buf, clen, mlen, 0x20U);
		--plen;
	}

	if (sign)
		utf8_do_printf_putc__(buf, clen, mlen, sign);

	while (ipl > 0)
		utf8_do_printf_putc__(buf, clen, mlen, icnvb[--ipl]);

	if (max > 0) 
	{
		utf8_do_printf_putc__(buf, clen, mlen, 0x2EU);

		while (fpl > 0)
			utf8_do_printf_putc__(buf, clen, mlen, fcnvb[--fpl]);
	}

	while (zlen > 0) 
	{
		utf8_do_printf_putc__(buf, clen, mlen, 0x30U);
		--zlen;
	}

	while (plen < 0) 
	{
		utf8_do_printf_putc__(buf, clen, mlen, 0x20U);
		++plen;
	}
}


inline static void utf8_do_printf_putc__(utf8_t* buf, size_t* clen, size_t mlen, utf8_t ch)
{
	if (*clen < mlen) buf[(*clen)++] = ch;
}


int32_t sb_vsnprintf_utf8(utf8_t* buf, size_t len, const utf8_t* fmt, va_list args)
{
	size_t ret = 0;

	buf[0] = 0x00U;

	utf8_do_printf__(buf, len, &ret, fmt, args);

	return (int32_t)ret;
}


int32_t sb_snprintf_utf8(utf8_t* buf, size_t len, const utf8_t* fmt, ...)
{
	int32_t ret = 0;
	va_list ap = NULL;

	va_start(ap, fmt);

	ret = sb_vsnprintf_utf8(buf, len, fmt, ap);

	va_end(ap);

	return ret;
}


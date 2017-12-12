// getuid.c - Get UID support for Windows.


#include "../config.h"


#if defined(SEC_OS_WINDOWS)


#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <sddl.h>


inline static uint64_t rotl64__(uint64_t x, uint64_t b)
{
	return (x << b) | (x >> (64 - b));
}

inline static void mix64__(uint64_t* v0, uint64_t* v1, uint64_t* v2, uint64_t* v3)
{
	*v0 += *v1;
	*v1 = rotl64__(*v1, 13);
	*v1 ^= *v0;
	*v0 = rotl64__(*v0, 32);
	*v2 += *v3;
	*v3 = rotl64__(*v3, 16);
	*v3 ^= *v2;
	*v0 += *v3;
	*v3 = rotl64__(*v3, 21);
	*v3 ^= *v0;
	*v2 += *v1;
	*v1 = rotl64__(*v1, 17);
	*v1 ^= *v2;
	*v2 = rotl64__(*v2, 32);
}

inline static uint64_t hash64__(const uint8_t* p, size_t n)
{
	uint64_t v0 = UINT64_C(0x736F6D6570736575);
	uint64_t v1 = UINT64_C(0x646F72616E646F6D);
	uint64_t v2 = UINT64_C(0x6C7967656E657261);
	uint64_t v3 = UINT64_C(0x7465646279746573);

	uint64_t k0 = n << 32;
	uint64_t k1 = ~(n << 32) ^ ~(uint64_t)p[n / 2];

	v0 ^= k0;
	v1 ^= k1;
	v2 ^= k0;
	v3 ^= k1;

	uint64_t b = n << 56;
	uint64_t t = (v0 << 12) | (v1 << 24) | (v2 << 36) | (v3 << 48);

	while (n-- > 0U)
	{
		t ^= (uint64_t)*p;

		switch (n % 8)
		{
		case 7: b |= t << 48;
		case 6: b |= t << 40;
		case 5: b |= t << 32;
		case 4: b |= t << 24;
		case 3: b |= t << 16;
		case 2: b |= t << 8;
		case 1: b |= t;
		case 0: break;
		}

		++p;
	}

	v3 ^= b;
	mix64__(&v0, &v1, &v2, &v3);
	mix64__(&v0, &v1, &v2, &v3);
	v0 ^= b;
	v2 ^= UINT64_C(0xFF);
	mix64__(&v0, &v1, &v2, &v3);
	mix64__(&v0, &v1, &v2, &v3);
	mix64__(&v0, &v1, &v2, &v3);
	mix64__(&v0, &v1, &v2, &v3);

	return (v0 ^ v1 ^ v2  ^ v3);
}


inline static PSID get_user_sid__(HANDLE token)
{
	if (token == NULL || token == INVALID_HANDLE_VALUE) return NULL;
	DWORD tkl = 0;
	GetTokenInformation(token, TokenUser, NULL, 0, &tkl);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return NULL;
	PTOKEN_USER ptok = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, tkl);
	if (ptok == NULL) return NULL;
	if (!GetTokenInformation(token, TokenUser, ptok, tkl, &tkl)) { HeapFree(GetProcessHeap(), 0, ptok); return NULL; }
	DWORD lsid = GetLengthSid(ptok->User.Sid);
	PSID psid = (PSID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lsid);
	if (psid == NULL) { HeapFree(GetProcessHeap(), 0, ptok); return NULL; }
	if (!CopySid(lsid, psid, ptok->User.Sid)) { HeapFree(GetProcessHeap(), 0, ptok); HeapFree(GetProcessHeap(), 0, psid); return NULL; }
	if (!IsValidSid(psid)) { HeapFree(GetProcessHeap(), 0, ptok); HeapFree(GetProcessHeap(), 0, psid); return NULL; }
	HeapFree(GetProcessHeap(), 0, ptok);
	return psid;
}


inline static uid_t get_token_uid__(HANDLE token, uint64_t* hash)
{
	uid_t u = -1;
	PSID sid = get_user_sid__(token);
	if (!sid) return u;
	LPSTR ssid = NULL;
	if (!ConvertSidToStringSidA(sid, &ssid)) { HeapFree(GetProcessHeap(), 0, sid); return u; }
	if (hash) { *hash = hash64__(ssid, strlen(ssid)); }
	LPCSTR p = strrchr(ssid, '-');
	if (p && isdigit(p[1])) { ++p; u = atoi(p); }
	HeapFree(GetProcessHeap(), 0, sid);
	LocalFree(ssid);
	return u;
}


uid_t getuid()
{
	uid_t u = -1;
	HANDLE token = NULL;
	HANDLE process = GetCurrentProcess();
	if (!OpenProcessToken(process, TOKEN_READ | TOKEN_QUERY_SOURCE, &token)) { CloseHandle(process); return u; }
	u = get_token_uid__(token, NULL);
	CloseHandle(token);
	CloseHandle(process);
	return u;
}


uint64_t getsidhash()
{
	uint64_t sh = 0;
	uid_t u = -1;
	HANDLE token = NULL;
	HANDLE process = GetCurrentProcess();
	if (!OpenProcessToken(process, TOKEN_READ | TOKEN_QUERY_SOURCE, &token)) { CloseHandle(process); return u; }
	u = get_token_uid__(token, &sh);
	CloseHandle(token);
	CloseHandle(process);
	return sh ^ u;
}


#endif // SEC_OS_WINDOWS


// getuid.c - Get UID support for Windows.


#include <stdint.h>
#include <stdlib.h>


#include "../config.h"


inline static uint64_t rotl64(uint64_t x, uint64_t b)
{
	return (x << b) | (x >> ((64 - b) & 63));
}


inline static void fmix64(uint64_t* v0, uint64_t* v1, uint64_t* v2, uint64_t* v3)
{
	*v0 += *v1, *v1 = rotl64(*v1, 13);
	*v1 ^= *v0, *v0 = rotl64(*v0, 32);
	*v2 += *v3, *v3 = rotl64(*v3, 16);
	*v3 ^= *v2, *v0 += *v3, *v3 = rotl64(*v3, 21);
	*v3 ^= *v0, *v2 += *v1, *v1 = rotl64(*v1, 17);
	*v1 ^= *v2, *v2 = rotl64(*v2, 32);
}


inline static uint64_t hash64(const uint8_t* p, size_t n)
{
	uint64_t v0 = UINT64_C(0x736F6D6570736575), v1 = UINT64_C(0x646F72616E646F6D), v2 = UINT64_C(0x6C7967656E657261), v3 = UINT64_C(0x7465646279746573);
	uint64_t k0 = n << 32, k1 = ~(n << p[0] % 64) ^ ~(uint64_t)p[n / 2];
	v0 ^= k0, v1 ^= k1, v2 ^= k0, v3 ^= k1;
	uint64_t b = n << 56, t = (v0 << 12) | (v1 << 24) | (v2 << 36) | (v3 << 48);

	while (n-- > 0)
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
	fmix64(&v0, &v1, &v2, &v3);
	fmix64(&v0, &v1, &v2, &v3);
	v0 ^= b;
	v2 ^= UINT64_C(0xFF);
	fmix64(&v0, &v1, &v2, &v3);
	fmix64(&v0, &v1, &v2, &v3);
	fmix64(&v0, &v1, &v2, &v3);
	fmix64(&v0, &v1, &v2, &v3);

	return (v0 ^ v1 ^ v2  ^ v3);
}


#if defined(SM_OS_WINDOWS)


#include <windows.h>
#include <sddl.h>


inline static PSID sm_get_user_sid(HANDLE token)
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


inline static uid_t sm_get_token_uid(HANDLE token, uint64_t* hash)
{
	uid_t u = -1;
	PSID sid = sm_get_user_sid(token);
	if (!sid) return u;
	LPSTR ssid = NULL;
	if (!ConvertSidToStringSidA(sid, &ssid)) { HeapFree(GetProcessHeap(), 0, sid); return u; }
	if (hash) { *hash = hash64(ssid, strlen(ssid)); }
	LPCSTR p = strrchr(ssid, '-');
	if (p && isdigit(p[1])) { ++p; u = atoi(p); }
	HeapFree(GetProcessHeap(), 0, sid);
	LocalFree(ssid);
	return u;
}


// Get thread ID.
exported uint64_t callconv sm_gettid()
{
	return (uint64_t)GetThreadId(GetCurrentThread());
}


// Get user ID.
exported uid_t callconv sm_getuid()
{
	uid_t u = -1;
	HANDLE token = NULL;
	HANDLE process = GetCurrentProcess();
	if (!OpenProcessToken(process, TOKEN_READ | TOKEN_QUERY_SOURCE, &token)) { CloseHandle(process); return u; }
	u = sm_get_token_uid(token, NULL);
	CloseHandle(token);
	CloseHandle(process);
	return u;
}


// Get hash of user name.
exported uint64_t callconv sm_getunh()
{
	uint64_t sh = 0;
	uid_t u = -1;
	HANDLE token = NULL;
	HANDLE process = GetCurrentProcess();
	if (!OpenProcessToken(process, TOKEN_READ | TOKEN_QUERY_SOURCE, &token)) { CloseHandle(process); return u; }
	u = sm_get_token_uid(token, &sh);
	CloseHandle(token);
	CloseHandle(process);
	return sh ^ (((uint64_t)u) << 21);
}

#else


#include <pwd.h>


// Get hash of user name.
exported uint64_t callconv sm_getunh()
{
	struct passwd* pw = getpwuid(geteuid());
	if (pw) return hash64(pw->pw_name, strlen(pw->pw_name));
	return -1;
}


#endif // SM_OS_WINDOWS


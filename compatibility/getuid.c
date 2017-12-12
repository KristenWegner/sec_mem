// getuid.c - Get UID support for Windows.


#include "../config.h"


#if defined(SEC_OS_WINDOWS)


#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <sddl.h>


inline static PSID get_user_sid__(HANDLE token)
{
	if (token == NULL || token == INVALID_HANDLE_VALUE) return NULL;
	DWORD tkl = 0;
	if (!GetTokenInformation(token, TokenUser, NULL, 0, &tkl)) return NULL;
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


inline static uid_t get_token_uid__(HANDLE token)
{
	uid_t u = -1;
	PSID sid = get_user_sid__(token);
	if (!sid) return u;
	LPSTR ssid = NULL;
	if (!ConvertSidToStringSidA(sid, &ssid)) { HeapFree(GetProcessHeap(), 0, sid); return u; }
	LPCSTR p = strrchr(ssid, '-');
	if (p && isdigit(p[1])) { ++p; u = atoi(p); }
	HeapFree(GetProcessHeap(), 0, sid);
	LocalFree(ssid);
	return u;
}


uid_t getuid()
{
	uid_t u = -1;
	HANDLE token = NULL, process = GetCurrentProcess();
	if (!OpenProcessToken(process, TOKEN_READ | TOKEN_QUERY_SOURCE, &token)) { CloseHandle(process); return u; }
	u = get_token_uid__(token);
	CloseHandle(token);
	CloseHandle(process);
	return u;
}


#endif // SEC_OS_WINDOWS


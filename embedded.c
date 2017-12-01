// embedded.c - Embedded machine code for obfuscated functions.


#include "config.h"
#include "embedded.h"


#if defined(SEC_OS_LINUX)
#include <sys/mman.h>
static int (*em_mp__)(void*,size_t,int) = mprotect;
#define em_mkex(a, r) { (r) = (em_mp__((void*)&((a)[0]), sizeof(a), PROT_READ|PROT_EXEC) == 0); } // Makes the specified page executable.
#elif defined(SEC_OS_WINDOWS)
#include <windows.h>
static int (__stdcall *em_mp__)(void*,size_t,unsigned long, unsigned long*) = VirtualProtect;
#define em_mkex(a, r) { unsigned long mx_dv__; (r) = (em_mp__((void*)&((a)[0]), sizeof(a), PAGE_EXECUTE_READ, &mx_dv__) != 0); /*if (r) FlushInstructionCache(GetCurrentProcess(), NULL, 0);*/ } // Makes the specified page executable.
#else
#error Dynamic code execution is not available.
#endif


// Obfuscated Names


#define HRDRND f7e4bb7a
#define RDRAND f8ad3ee5
#define SHRGML fc547592
#define FS20SD ff2fded4
#define FS20RG f9db7e10
#define KN02SD f516a510
#define KN02RG fb3bd986


// Machine Code

// dumpbin /DISASM:BYTES $1.obj > $1.obj.txt

static uint8_t HRDRND[] = { 0x53, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x0F, 0xA2, 0x0F, 0xBA, 0xE1, 0x1E, 0x73, 0x09, 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x5B, 0xC3, 0x48, 0x2B, 0xC0, 0xEB, 0xF2 };
static uint8_t RDRAND[] = { 0x48, 0x0F, 0xC7, 0xF0, 0x73, 0xFA, 0xC3 };
static uint8_t SHRGML[] = { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x48, 0x89, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0xF2, 0x48, 0x8B, 0xC1, 0x33, 0xD2, 0x45, 0x33, 0xDB, 0x49, 0xF7, 0xF1, 0x4D, 0x8B, 0xD0, 0x48, 0x8B, 0xDA, 0x48, 0x8B, 0xF8, 0x48, 0x85, 0xD2, 0x75, 0x05, 0x41, 0x8B, 0xDB, 0xEB, 0x43, 0x33, 0xD2, 0x49, 0x8B, 0xC2, 0x48, 0xF7, 0xF3, 0x48, 0x8B, 0xC8, 0x4C, 0x8B, 0xC2, 0x33, 0xD2, 0x48, 0x8B, 0xC6, 0x48, 0xF7, 0xF1, 0x4B, 0x8D, 0x0C, 0x12, 0x49, 0x0F, 0xAF, 0xC0, 0x48, 0x2B, 0xC8, 0x48, 0x8B, 0xC1, 0x49, 0x2B, 0xC2, 0x49, 0x3B, 0xCA, 0x48, 0x0F, 0x42, 0xC1, 0x48, 0x0F, 0xAF, 0xD3, 0x48, 0x8D, 0x0C, 0x10, 0x48, 0x8B, 0xD9, 0x49, 0x2B, 0xDA, 0x49, 0x3B, 0xCA, 0x48, 0x0F, 0x42, 0xD9, 0x4D, 0x85, 0xC9, 0x75, 0x05, 0x4D, 0x8B, 0xCB, 0xEB, 0x43, 0x33, 0xD2, 0x49, 0x8B, 0xC2, 0x49, 0xF7, 0xF1, 0x48, 0x8B, 0xC8, 0x4C, 0x8B, 0xC2, 0x33, 0xD2, 0x48, 0x8B, 0xC6, 0x48, 0xF7, 0xF1, 0x4B, 0x8D, 0x0C, 0x12, 0x49, 0x0F, 0xAF, 0xC0, 0x48, 0x2B, 0xC8, 0x48, 0x8B, 0xC1, 0x49, 0x2B, 0xC2, 0x49, 0x3B, 0xCA, 0x48, 0x0F, 0x42, 0xC1, 0x49, 0x0F, 0xAF, 0xD1, 0x48, 0x8D, 0x0C, 0x10, 0x4C, 0x8B, 0xC9, 0x4D, 0x2B, 0xCA, 0x49, 0x3B, 0xCA, 0x4C, 0x0F, 0x42, 0xC9, 0x48, 0x85, 0xFF, 0x74, 0x43, 0x33, 0xD2, 0x49, 0x8B, 0xC2, 0x48, 0xF7, 0xF7, 0x48, 0x8B, 0xC8, 0x4C, 0x8B, 0xC2, 0x33, 0xD2, 0x49, 0x8B, 0xC1, 0x48, 0xF7, 0xF1, 0x4B, 0x8D, 0x0C, 0x12, 0x4C, 0x0F, 0xAF, 0xC0, 0x49, 0x2B, 0xC8, 0x48, 0x8B, 0xC1, 0x49, 0x2B, 0xC2, 0x49, 0x3B, 0xCA, 0x48, 0x0F, 0x42, 0xC1, 0x48, 0x0F, 0xAF, 0xD7, 0x48, 0x8D, 0x0C, 0x10, 0x4C, 0x8B, 0xD9, 0x4D, 0x2B, 0xDA, 0x49, 0x3B, 0xCA, 0x4C, 0x0F, 0x42, 0xD9, 0x48, 0x8B, 0x74, 0x24, 0x10, 0x49, 0x8D, 0x0C, 0x1B, 0x48, 0x8B, 0x5C, 0x24, 0x08, 0x48, 0x8B, 0xC1, 0x48, 0x8B, 0x7C, 0x24, 0x18, 0x49, 0x2B, 0xC2, 0x49, 0x3B, 0xCA, 0x48, 0x0F, 0x42, 0xC1, 0xC3 };
static uint8_t FS20SD[] = { 0x4C, 0x8B, 0xC2, 0x4C, 0x8B, 0xC9, 0x48, 0xB8, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xB9, 0x01, 0x00, 0x00, 0x00, 0x48, 0xF7, 0xE2, 0x49, 0x8B, 0xC0, 0x48, 0x2B, 0xC2, 0x48, 0xD1, 0xE8, 0x48, 0x03, 0xC2, 0x48, 0xC1, 0xE8, 0x1E, 0x48, 0x69, 0xC0, 0xFF, 0xFF, 0xFF, 0x7F, 0x4C, 0x3B, 0xC0, 0x49, 0x0F, 0x45, 0xC8, 0x0F, 0xBA, 0xF1, 0x1F, 0x49, 0x89, 0x09, 0xC3 };
static uint8_t FS20RG[] = { 0x4C, 0x8B, 0xC9, 0x49, 0xBA, 0x69, 0xD8, 0x3B, 0x4C, 0xC8, 0x89, 0x47, 0x5E, 0x49, 0x8B, 0xC2, 0x48, 0xF7, 0x21, 0x48, 0x69, 0x09, 0x8F, 0xBC, 0x00, 0x00, 0x48, 0xC1, 0xEA, 0x0E, 0x48, 0x69, 0xC2, 0xFF, 0xFF, 0xFF, 0x7F, 0x48, 0x2B, 0xC8, 0x79, 0x0C, 0x48, 0x8D, 0x81, 0xFF, 0xFF, 0xFF, 0x7F, 0x49, 0x89, 0x01, 0xEB, 0x03, 0x49, 0x89, 0x09, 0x45, 0x8B, 0x01, 0x49, 0x8B, 0xC2, 0x49, 0xF7, 0x21, 0x48, 0xC1, 0xEA, 0x0E, 0x48, 0x69, 0xC2, 0xFF, 0xFF, 0xFF, 0x7F, 0x49, 0x69, 0x11, 0x8F, 0xBC, 0x00, 0x00, 0x48, 0x2B, 0xD0, 0x79, 0x0C, 0x48, 0x8D, 0x82, 0xFF, 0xFF, 0xFF, 0x7F, 0x49, 0x89, 0x01, 0xEB, 0x03, 0x49, 0x89, 0x11, 0x49, 0x8B, 0x09, 0x48, 0x8D, 0x04, 0x8D, 0x00, 0x00, 0x00, 0x00, 0x44, 0x33, 0xC0, 0x49, 0x8B, 0xC2, 0x48, 0xF7, 0xE1, 0x44, 0x89, 0x44, 0x24, 0x08, 0x48, 0xC1, 0xEA, 0x0E, 0x48, 0x69, 0xC2, 0xFF, 0xFF, 0xFF, 0x7F, 0x48, 0x69, 0xD1, 0x8F, 0xBC, 0x00, 0x00, 0x48, 0x2B, 0xD0, 0x79, 0x0C, 0x48, 0x8D, 0x82, 0xFF, 0xFF, 0xFF, 0x7F, 0x49, 0x89, 0x01, 0xEB, 0x03, 0x49, 0x89, 0x11, 0x45, 0x8B, 0x01, 0x49, 0x8B, 0xC2, 0x49, 0xF7, 0x21, 0x48, 0xC1, 0xEA, 0x0E, 0x48, 0x69, 0xC2, 0xFF, 0xFF, 0xFF, 0x7F, 0x49, 0x69, 0x11, 0x8F, 0xBC, 0x00, 0x00, 0x48, 0x2B, 0xD0, 0x79, 0x1C, 0x48, 0x8D, 0x82, 0xFF, 0xFF, 0xFF, 0x7F, 0x49, 0x89, 0x01, 0x48, 0xC1, 0xE0, 0x02, 0x44, 0x33, 0xC0, 0x44, 0x89, 0x44, 0x24, 0x0C, 0x48, 0x8B, 0x44, 0x24, 0x08, 0xC3, 0x48, 0x8B, 0xC2, 0x49, 0x89, 0x11, 0x48, 0xC1, 0xE0, 0x02, 0x44, 0x33, 0xC0, 0x44, 0x89, 0x44, 0x24, 0x0C, 0x48, 0x8B, 0x44, 0x24, 0x08, 0xC3 };
static uint8_t KN02SD[] = { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48, 0x81, 0xEC, 0x40, 0x06, 0x00, 0x00, 0x48, 0x85, 0xD2, 0x48, 0x8D, 0x99, 0x8C, 0x1F, 0x00, 0x00, 0x41, 0xB9, 0x2F, 0xCB, 0x04, 0x00, 0x48, 0x8B, 0xF9, 0x4C, 0x0F, 0x45, 0xCA, 0x49, 0x8D, 0x51, 0x02, 0x81, 0xE2, 0xFE, 0xFF, 0xFF, 0x3F, 0x33, 0xF6, 0x44, 0x8B, 0xC6, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4A, 0x89, 0x14, 0xC4, 0x48, 0x8D, 0x04, 0x12, 0x49, 0xFF, 0xC0, 0x48, 0x8D, 0x90, 0x02, 0x00, 0x00, 0xC0, 0x48, 0x3D, 0x00, 0x00, 0x00, 0x40, 0x48, 0x0F, 0x42, 0xD0, 0x49, 0x83, 0xF8, 0x64, 0x7C, 0xDE, 0x48, 0xFF, 0x44, 0x24, 0x08, 0x41, 0xBA, 0x45, 0x00, 0x00, 0x00, 0x41, 0x81, 0xE1, 0xFF, 0xFF, 0xFF, 0x3F, 0x0F, 0x1F, 0x40, 0x00, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x94, 0x24, 0x08, 0x03, 0x00, 0x00, 0x41, 0xB8, 0x21, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x84, 0x24, 0x20, 0x06, 0x00, 0x00, 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x4A, 0x10, 0x48, 0x8D, 0x52, 0xE8, 0x48, 0x89, 0x48, 0x10, 0x48, 0x89, 0x70, 0x08, 0x48, 0x8D, 0x40, 0xD0, 0x48, 0x8B, 0x4A, 0x20, 0x48, 0x89, 0x48, 0x30, 0x48, 0x89, 0x70, 0x28, 0x48, 0x8B, 0x4A, 0x18, 0x48, 0x89, 0x48, 0x20, 0x48, 0x89, 0x70, 0x18, 0x49, 0x83, 0xE8, 0x01, 0x75, 0xCE, 0x48, 0x8D, 0x94, 0x24, 0x10, 0x03, 0x00, 0x00, 0x41, 0xB8, 0x63, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x8A, 0x20, 0x03, 0x00, 0x00, 0x48, 0x8B, 0x82, 0x28, 0x01, 0x00, 0x00, 0x48, 0x8D, 0x52, 0xF8, 0x48, 0x2B, 0xC1, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x82, 0x30, 0x01, 0x00, 0x00, 0x48, 0x8B, 0x42, 0x08, 0x48, 0x2B, 0xC1, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0x08, 0x49, 0x83, 0xE8, 0x01, 0x75, 0xC9, 0x41, 0xF6, 0xC1, 0x01, 0x74, 0x50, 0x48, 0x8D, 0x8C, 0x24, 0x18, 0x03, 0x00, 0x00, 0x41, 0x8D, 0x50, 0x32, 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x01, 0x48, 0x89, 0x41, 0x08, 0x48, 0x8B, 0x41, 0xF8, 0x48, 0x89, 0x01, 0x48, 0x8D, 0x49, 0xF0, 0x48, 0x83, 0xEA, 0x01, 0x75, 0xE8, 0x48, 0x8B, 0x84, 0x24, 0x20, 0x03, 0x00, 0x00, 0x48, 0x8B, 0x8C, 0x24, 0x28, 0x01, 0x00, 0x00, 0x48, 0x2B, 0xC8, 0x48, 0x89, 0x04, 0x24, 0x81, 0xE1, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x8C, 0x24, 0x28, 0x01, 0x00, 0x00, 0x4D, 0x85, 0xC9, 0x74, 0x05, 0x49, 0xD1, 0xF9, 0xEB, 0x03, 0x41, 0xFF, 0xCA, 0x45, 0x85, 0xD2, 0x0F, 0x85, 0xFD, 0xFE, 0xFF, 0xFF, 0x48, 0x8D, 0x8B, 0xF8, 0x01, 0x00, 0x00, 0x48, 0x8D, 0x14, 0x24, 0x45, 0x8D, 0x42, 0x09, 0x48, 0x8B, 0x02, 0x48, 0x8D, 0x52, 0x20, 0x48, 0x89, 0x01, 0x48, 0x8D, 0x49, 0x20, 0x48, 0x8B, 0x42, 0xE8, 0x48, 0x89, 0x41, 0xE8, 0x48, 0x8B, 0x42, 0xF0, 0x48, 0x89, 0x41, 0xF0, 0x48, 0x8B, 0x42, 0xF8, 0x48, 0x89, 0x41, 0xF8, 0x49, 0x83, 0xE8, 0x01, 0x75, 0xD4, 0x48, 0x8B, 0x02, 0x41, 0xB8, 0x0F, 0x00, 0x00, 0x00, 0x48, 0x89, 0x01, 0x48, 0x8D, 0x94, 0x24, 0x28, 0x01, 0x00, 0x00, 0x48, 0x8B, 0xCB, 0x66, 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x02, 0x48, 0x8D, 0x52, 0x20, 0x48, 0x89, 0x01, 0x48, 0x8D, 0x49, 0x20, 0x48, 0x8B, 0x42, 0xE8, 0x48, 0x89, 0x41, 0xE8, 0x48, 0x8B, 0x42, 0xF0, 0x48, 0x89, 0x41, 0xF0, 0x48, 0x8B, 0x42, 0xF8, 0x48, 0x89, 0x41, 0xF8, 0x49, 0x83, 0xE8, 0x01, 0x75, 0xD4, 0x48, 0x8B, 0x02, 0x4C, 0x8D, 0x94, 0x24, 0x18, 0x03, 0x00, 0x00, 0x48, 0x89, 0x01, 0x45, 0x8D, 0x58, 0x0A, 0x48, 0x8B, 0x42, 0x08, 0x4C, 0x2B, 0xD3, 0x48, 0x89, 0x41, 0x08, 0x48, 0x8B, 0x42, 0x10, 0x48, 0x89, 0x41, 0x10, 0x0F, 0x1F, 0x40, 0x00, 0x66, 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x0C, 0x24, 0x48, 0x8B, 0xD3, 0x41, 0xB8, 0x19, 0x00, 0x00, 0x00, 0x0F, 0x1F, 0x00, 0x48, 0x8B, 0x02, 0x48, 0x8D, 0x52, 0x20, 0x48, 0x89, 0x01, 0x48, 0x8D, 0x49, 0x20, 0x48, 0x8B, 0x42, 0xE8, 0x48, 0x89, 0x41, 0xE8, 0x48, 0x8B, 0x42, 0xF0, 0x48, 0x89, 0x41, 0xF0, 0x48, 0x8B, 0x42, 0xF8, 0x48, 0x89, 0x41, 0xF8, 0x49, 0x83, 0xE8, 0x01, 0x75, 0xD4, 0x48, 0x8D, 0x8C, 0x24, 0xF8, 0x01, 0x00, 0x00, 0x41, 0x8D, 0x50, 0x21, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x81, 0x08, 0xFE, 0xFF, 0xFF, 0x2B, 0x01, 0x48, 0x8D, 0x49, 0x18, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x81, 0x10, 0x01, 0x00, 0x00, 0x8B, 0x81, 0xF8, 0xFD, 0xFF, 0xFF, 0x2B, 0x41, 0xF0, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x81, 0x18, 0x01, 0x00, 0x00, 0x8B, 0x81, 0x00, 0xFE, 0xFF, 0xFF, 0x2B, 0x41, 0xF8, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x81, 0x20, 0x01, 0x00, 0x00, 0x48, 0x83, 0xEA, 0x01, 0x75, 0xB8, 0x41, 0xB8, 0xA2, 0x00, 0x00, 0x00, 0x48, 0x8B, 0xD3, 0x45, 0x8D, 0x48, 0x83, 0x66, 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x8B, 0x0C, 0x12, 0x48, 0x8D, 0x52, 0x08, 0x41, 0x8B, 0xC0, 0x41, 0xFF, 0xC0, 0x2B, 0x0C, 0xC4, 0x81, 0xE1, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x4A, 0xF8, 0x49, 0x83, 0xE9, 0x01, 0x75, 0xDF, 0x4C, 0x8D, 0x84, 0x24, 0x40, 0x04, 0x00, 0x00, 0x48, 0x8B, 0xCB, 0x4C, 0x2B, 0xC3, 0x41, 0x8D, 0x51, 0x3F, 0x0F, 0x1F, 0x40, 0x00, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x8B, 0x04, 0x08, 0x2B, 0x01, 0x48, 0x8D, 0x49, 0x08, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x81, 0x20, 0x01, 0x00, 0x00, 0x48, 0x83, 0xEA, 0x01, 0x75, 0xE4, 0x49, 0x83, 0xEB, 0x01, 0x0F, 0x85, 0xEA, 0xFE, 0xFF, 0xFF, 0x4C, 0x8D, 0x9C, 0x24, 0x40, 0x06, 0x00, 0x00, 0x89, 0x37, 0x49, 0x8B, 0x5B, 0x10, 0x49, 0x8B, 0x73, 0x18, 0x49, 0x8B, 0xE3, 0x5F, 0xC3 };
static uint8_t KN02RG[] = { 0x48, 0x89, 0x5C, 0x24, 0x18, 0x48, 0x89, 0x6C, 0x24, 0x20, 0x48, 0x89, 0x4C, 0x24, 0x08, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57, 0x8B, 0x29, 0x4C, 0x8D, 0x49, 0x04, 0xBE, 0xCC, 0x03, 0x00, 0x00, 0x41, 0xBC, 0x32, 0x00, 0x00, 0x00, 0x41, 0xBA, 0x2F, 0x01, 0x00, 0x00, 0x45, 0x8D, 0x74, 0x24, 0xF3, 0x44, 0x8D, 0x6E, 0x4A, 0x85, 0xED, 0x0F, 0x85, 0x13, 0x01, 0x00, 0x00, 0x4D, 0x8D, 0x81, 0x88, 0x1F, 0x00, 0x00, 0x41, 0x8B, 0xD4, 0x4D, 0x2B, 0xC1, 0x49, 0x8D, 0x49, 0x08, 0x0F, 0x1F, 0x40, 0x00, 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x8B, 0x44, 0x08, 0xF8, 0x48, 0x89, 0x41, 0xF8, 0x49, 0x8B, 0x04, 0x08, 0x48, 0x89, 0x01, 0x48, 0x8D, 0x49, 0x10, 0x48, 0x83, 0xEA, 0x01, 0x75, 0xE6, 0x49, 0x8D, 0x91, 0x30, 0x03, 0x00, 0x00, 0x4D, 0x8B, 0xC2, 0x49, 0x8D, 0x89, 0xF8, 0x01, 0x00, 0x00, 0x0F, 0x1F, 0x44, 0x00, 0x00, 0x8B, 0x81, 0x08, 0xFE, 0xFF, 0xFF, 0x48, 0x8D, 0x52, 0x18, 0x2B, 0x01, 0x48, 0x8D, 0x49, 0x18, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xD8, 0x8B, 0x81, 0xF8, 0xFD, 0xFF, 0xFF, 0x2B, 0x41, 0xF0, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xE0, 0x8B, 0x81, 0x00, 0xFE, 0xFF, 0xFF, 0x2B, 0x41, 0xF8, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xE8, 0x49, 0x83, 0xE8, 0x01, 0x75, 0xBD, 0x44, 0x8B, 0xC6, 0x4D, 0x8D, 0x99, 0x88, 0x1F, 0x00, 0x00, 0x49, 0x8B, 0xDE, 0x45, 0x8B, 0xD6, 0x0F, 0x1F, 0x40, 0x00, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x8B, 0xC0, 0x41, 0x8D, 0x48, 0xC1, 0x41, 0x8B, 0x14, 0xC9, 0x4D, 0x8D, 0x5B, 0x08, 0x41, 0xFF, 0xC0, 0x41, 0x2B, 0x14, 0xC1, 0x81, 0xE2, 0xFF, 0xFF, 0xFF, 0x3F, 0x49, 0x89, 0x53, 0xF8, 0x48, 0x83, 0xEB, 0x01, 0x75, 0xDA, 0x45, 0x8B, 0xC5, 0x4D, 0x8D, 0x99, 0xB0, 0x20, 0x00, 0x00, 0x41, 0x8D, 0x40, 0x9C, 0x41, 0x8B, 0x04, 0xC1, 0x41, 0x8D, 0x4A, 0xDB, 0x41, 0x2B, 0x84, 0xC9, 0x88, 0x1F, 0x00, 0x00, 0x4D, 0x8D, 0x5B, 0x08, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x45, 0x8D, 0x40, 0x01, 0x41, 0xFF, 0xC2, 0x49, 0x89, 0x43, 0xF8, 0x41, 0x83, 0xFA, 0x64, 0x72, 0xD2, 0x41, 0xBA, 0x2F, 0x01, 0x00, 0x00, 0x4D, 0x8B, 0x3C, 0xE9, 0xB8, 0x1F, 0x85, 0xEB, 0x51, 0xFF, 0xC5, 0xF7, 0xE5, 0xC1, 0xEA, 0x05, 0x6B, 0xC2, 0x64, 0x2B, 0xE8, 0x48, 0x8B, 0x44, 0x24, 0x38, 0x89, 0x28, 0x0F, 0x85, 0x08, 0x01, 0x00, 0x00, 0x4D, 0x8D, 0x81, 0x88, 0x1F, 0x00, 0x00, 0x49, 0x8B, 0xD4, 0x4D, 0x2B, 0xC1, 0x49, 0x8D, 0x49, 0x08, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x8B, 0x44, 0x08, 0xF8, 0x48, 0x89, 0x41, 0xF8, 0x49, 0x8B, 0x04, 0x08, 0x48, 0x89, 0x01, 0x48, 0x8D, 0x49, 0x10, 0x48, 0x83, 0xEA, 0x01, 0x75, 0xE6, 0x49, 0x8D, 0x91, 0x30, 0x03, 0x00, 0x00, 0x4D, 0x8B, 0xC2, 0x49, 0x8D, 0x89, 0xF8, 0x01, 0x00, 0x00, 0x0F, 0x1F, 0x44, 0x00, 0x00, 0x8B, 0x81, 0x08, 0xFE, 0xFF, 0xFF, 0x48, 0x8D, 0x52, 0x18, 0x2B, 0x01, 0x48, 0x8D, 0x49, 0x18, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xD8, 0x8B, 0x81, 0xF8, 0xFD, 0xFF, 0xFF, 0x2B, 0x41, 0xF0, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xE0, 0x8B, 0x81, 0x00, 0xFE, 0xFF, 0xFF, 0x2B, 0x41, 0xF8, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xE8, 0x49, 0x83, 0xE8, 0x01, 0x75, 0xBD, 0x44, 0x8B, 0xC6, 0x4D, 0x8D, 0x99, 0x88, 0x1F, 0x00, 0x00, 0x49, 0x8B, 0xDE, 0x45, 0x8B, 0xD6, 0x0F, 0x1F, 0x40, 0x00, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x8B, 0xC0, 0x41, 0x8D, 0x48, 0xC1, 0x41, 0x8B, 0x14, 0xC9, 0x4D, 0x8D, 0x5B, 0x08, 0x41, 0xFF, 0xC0, 0x41, 0x2B, 0x14, 0xC1, 0x81, 0xE2, 0xFF, 0xFF, 0xFF, 0x3F, 0x49, 0x89, 0x53, 0xF8, 0x48, 0x83, 0xEB, 0x01, 0x75, 0xDA, 0x45, 0x8B, 0xC5, 0x4D, 0x8D, 0x99, 0xB0, 0x20, 0x00, 0x00, 0x41, 0x8D, 0x40, 0x9C, 0x41, 0x8B, 0x04, 0xC1, 0x41, 0x8D, 0x4A, 0xDB, 0x41, 0x2B, 0x84, 0xC9, 0x88, 0x1F, 0x00, 0x00, 0x4D, 0x8D, 0x5B, 0x08, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x45, 0x8D, 0x40, 0x01, 0x41, 0xFF, 0xC2, 0x49, 0x89, 0x43, 0xF8, 0x41, 0x83, 0xFA, 0x64, 0x72, 0xD2, 0x49, 0x8B, 0x0C, 0xE9, 0xB8, 0x1F, 0x85, 0xEB, 0x51, 0xFF, 0xC5, 0xF7, 0xE5, 0xC1, 0xEA, 0x05, 0x6B, 0xC2, 0x64, 0x2B, 0xE8, 0x48, 0x8B, 0x44, 0x24, 0x38, 0x89, 0x28, 0x48, 0x8D, 0x04, 0x8D, 0x00, 0x00, 0x00, 0x00, 0x41, 0x33, 0xC7, 0x89, 0x44, 0x24, 0x40, 0x85, 0xED, 0x0F, 0x85, 0x0D, 0x01, 0x00, 0x00, 0x4D, 0x8D, 0x81, 0x88, 0x1F, 0x00, 0x00, 0x49, 0x8B, 0xD4, 0x4D, 0x2B, 0xC1, 0x49, 0x8D, 0x49, 0x08, 0x0F, 0x1F, 0x40, 0x00, 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x8B, 0x44, 0x08, 0xF8, 0x48, 0x89, 0x41, 0xF8, 0x49, 0x8B, 0x04, 0x08, 0x48, 0x89, 0x01, 0x48, 0x8D, 0x49, 0x10, 0x48, 0x83, 0xEA, 0x01, 0x75, 0xE6, 0x49, 0x8D, 0x91, 0x30, 0x03, 0x00, 0x00, 0x41, 0xB8, 0x2F, 0x01, 0x00, 0x00, 0x49, 0x8D, 0x89, 0xF8, 0x01, 0x00, 0x00, 0x66, 0x90, 0x8B, 0x81, 0x08, 0xFE, 0xFF, 0xFF, 0x48, 0x8D, 0x52, 0x18, 0x2B, 0x01, 0x48, 0x8D, 0x49, 0x18, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xD8, 0x8B, 0x81, 0xF8, 0xFD, 0xFF, 0xFF, 0x2B, 0x41, 0xF0, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xE0, 0x8B, 0x81, 0x00, 0xFE, 0xFF, 0xFF, 0x2B, 0x41, 0xF8, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xE8, 0x49, 0x83, 0xE8, 0x01, 0x75, 0xBD, 0x44, 0x8B, 0xC6, 0x4D, 0x8D, 0x99, 0x88, 0x1F, 0x00, 0x00, 0x49, 0x8B, 0xDE, 0x45, 0x8B, 0xD6, 0x0F, 0x1F, 0x40, 0x00, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x8B, 0xC0, 0x41, 0x8D, 0x48, 0xC1, 0x41, 0x8B, 0x14, 0xC9, 0x4D, 0x8D, 0x5B, 0x08, 0x41, 0xFF, 0xC0, 0x41, 0x2B, 0x14, 0xC1, 0x81, 0xE2, 0xFF, 0xFF, 0xFF, 0x3F, 0x49, 0x89, 0x53, 0xF8, 0x48, 0x83, 0xEB, 0x01, 0x75, 0xDA, 0x45, 0x8B, 0xC5, 0x4D, 0x8D, 0x99, 0xB0, 0x20, 0x00, 0x00, 0x41, 0x8D, 0x40, 0x9C, 0x41, 0x8B, 0x04, 0xC1, 0x41, 0x8D, 0x4A, 0xDB, 0x41, 0x2B, 0x84, 0xC9, 0x88, 0x1F, 0x00, 0x00, 0x4D, 0x8D, 0x5B, 0x08, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x45, 0x8D, 0x40, 0x01, 0x41, 0xFF, 0xC2, 0x49, 0x89, 0x43, 0xF8, 0x41, 0x83, 0xFA, 0x64, 0x72, 0xD2, 0x4D, 0x8B, 0x3C, 0xE9, 0x8D, 0x7D, 0x01, 0x48, 0x8B, 0x6C, 0x24, 0x38, 0xB8, 0x1F, 0x85, 0xEB, 0x51, 0xF7, 0xE7, 0xC1, 0xEA, 0x05, 0x6B, 0xC2, 0x64, 0x2B, 0xF8, 0x89, 0x7D, 0x00, 0x0F, 0x85, 0xEE, 0x00, 0x00, 0x00, 0x49, 0x8D, 0x91, 0x88, 0x1F, 0x00, 0x00, 0x49, 0x2B, 0xD1, 0x49, 0x8D, 0x49, 0x08, 0x48, 0x8B, 0x44, 0x0A, 0xF8, 0x48, 0x89, 0x41, 0xF8, 0x48, 0x8B, 0x04, 0x0A, 0x48, 0x89, 0x01, 0x48, 0x8D, 0x49, 0x10, 0x49, 0x83, 0xEC, 0x01, 0x75, 0xE6, 0x49, 0x8D, 0x91, 0x30, 0x03, 0x00, 0x00, 0xBB, 0x2F, 0x01, 0x00, 0x00, 0x49, 0x8D, 0x89, 0xF8, 0x01, 0x00, 0x00, 0x0F, 0x1F, 0x00, 0x8B, 0x81, 0x08, 0xFE, 0xFF, 0xFF, 0x48, 0x8D, 0x52, 0x18, 0x2B, 0x01, 0x48, 0x8D, 0x49, 0x18, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xD8, 0x8B, 0x81, 0xF8, 0xFD, 0xFF, 0xFF, 0x2B, 0x41, 0xF0, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xE0, 0x8B, 0x81, 0x00, 0xFE, 0xFF, 0xFF, 0x2B, 0x41, 0xF8, 0x25, 0xFF, 0xFF, 0xFF, 0x3F, 0x48, 0x89, 0x42, 0xE8, 0x48, 0x83, 0xEB, 0x01, 0x75, 0xBD, 0x4D, 0x8D, 0x81, 0x88, 0x1F, 0x00, 0x00, 0x45, 0x8B, 0xD6, 0x0F, 0x1F, 0x00, 0x8B, 0xC6, 0x8D, 0x4E, 0xC1, 0x41, 0x8B, 0x14, 0xC9, 0x4D, 0x8D, 0x40, 0x08, 0xFF, 0xC6, 0x41, 0x2B, 0x14, 0xC1, 0x81, 0xE2, 0xFF, 0xFF, 0xFF, 0x3F, 0x49, 0x89, 0x50, 0xF8, 0x49, 0x83, 0xEE, 0x01, 0x75, 0xDD, 0x4D, 0x8D, 0x99, 0xB0, 0x20, 0x00, 0x00, 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00, 0x41, 0x8D, 0x55, 0x9C, 0x45, 0x8B, 0x04, 0xD1, 0x41, 0x8D, 0x42, 0xDB, 0x45, 0x2B, 0x84, 0xC1, 0x88, 0x1F, 0x00, 0x00, 0x4D, 0x8D, 0x5B, 0x08, 0x41, 0x81, 0xE0, 0xFF, 0xFF, 0xFF, 0x3F, 0x45, 0x8D, 0x6D, 0x01, 0x41, 0xFF, 0xC2, 0x4D, 0x89, 0x43, 0xF8, 0x41, 0x83, 0xFA, 0x64, 0x72, 0xD0, 0x4D, 0x8B, 0x04, 0xF9, 0x8D, 0x4F, 0x01, 0x48, 0x8B, 0x5C, 0x24, 0x48, 0xB8, 0x1F, 0x85, 0xEB, 0x51, 0xF7, 0xE1, 0xC1, 0xEA, 0x05, 0x6B, 0xC2, 0x64, 0x2B, 0xC8, 0x4A, 0x8D, 0x04, 0x85, 0x00, 0x00, 0x00, 0x00, 0x41, 0x33, 0xC7, 0x89, 0x4D, 0x00, 0x48, 0x8B, 0x6C, 0x24, 0x50, 0x89, 0x44, 0x24, 0x44, 0x48, 0x8B, 0x44, 0x24, 0x40, 0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C, 0x5F, 0x5E, 0xC3 };


// Methods


uint64_t sec_get_op(uint16_t op)
{
	uint8_t r = 0;

#define em_load(a) em_mkex(a, r)

	switch (op)
	{
	case SEC_OP_HRDRND: em_load(HRDRND); if (r) return (uint64_t)(void*)&HRDRND[0]; return 0ULL;
	case SEC_OP_RDRAND: em_load(RDRAND); if (r) return (uint64_t)(void*)&RDRAND[0]; return 0ULL;
	case SEC_OP_SHRGML: em_load(SHRGML); if (r) return (uint64_t)(void*)&SHRGML[0]; return 0ULL;
	case SEC_OP_FS20SD: em_load(FS20SD); if (r) return (uint64_t)(void*)&FS20SD[0]; return 0ULL;
	case SEC_OP_FS20RG: em_load(FS20RG); if (r) return (uint64_t)(void*)&FS20RG[0]; return 0ULL;
	case SEC_OP_KN02SD: em_load(KN02SD); if (r) return (uint64_t)(void*)&KN02SD[0]; return 0ULL;
	case SEC_OP_KN02RG: em_load(KN02RG); if (r) return (uint64_t)(void*)&KN02RG[0]; return 0ULL;
	default: return 0ULL;
	}

#undef em_load

}


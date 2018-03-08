// cfg.h - Configuration settings.


#ifndef INCLUDE_CFG_H
#define INCLUDE_CFG_H 1


#define TCC_VERSION					"1.0.0.1"


// Target selection.


#define UC_TARG_BITS_00
#define UC_TARG_BITS_32
#define UC_TARG_BITS_64

#define UC_TARG_ARCH_ARM
#define UC_TARG_ARCH_X86

#define UC_TARG_FORM_PEX
#define UC_TARG_FORM_ELF
#define UC_TARG_FORM_COF


//#define TCC_TARGET_I386			1 // x86-32 target.
#define TCC_TARGET_X86_64			1 // x86-64 target.
//#define TCC_TARGET_ARM			1 // ARMv4 target.
//#define TCC_ARM_EABI				1 // ARMv4 EABI target.
//#define TCC_ARM_VFP				1 // ARMv4 VFP target.
//#define TCC_TARGET_ARM64			1 // ARMv8 target.


#define TCC_TARGET_PE				1 // PE output format.
//#define TCC_TARGET_ELF			1 // ELF output format.
//#define TCC_TARGET_COFF			1 // COFF output format.


#define TCC_TARGET_WINDOWS			1 // Windows OS runtime.
//#define TCC_TARGET_LINUX			1 // Linux OS runtime.
//#define TCC_TARGET_UNIX			1 // Generic UNIX OS runtime.


#if defined(TCC_TARGET_I386)
#include "cpu/x86/def.h"
#elif defined(TCC_TARGET_X86_64)
#include "cpu/x86/64/def.h"
#elif defined(TCC_TARGET_ARM)
#include "cpu/arm/def.h"
#elif defined(TCC_TARGET_ARM64)
#include "cpu/arm/64/def.h"
#endif


// Limits.


#define INCLUDE_STACK_SIZE			32
#define IFDEF_STACK_SIZE			64
#define VSTACK_SIZE					256
#define STRING_MAX_SIZE				1024
#define TOKSTR_MAX_SIZE				256
#define PACK_STACK_SIZE				8
#define TOK_HASH_SIZE				16384 // Must be a power of 2.
#define TOK_ALLOC_INCR				512  // Must be a power of 2.
#define TOK_MAX_SIZE				4 // Maximum token size in integer units, when stored in a string.
#define CACHED_INCLUDES_HASH_SIZE	32
#define MAX_ASM_OPERANDS			30
#define IO_BUF_SIZE					8192


// Path configuration.


#ifndef CONFIG_SYSROOT
#define CONFIG_SYSROOT				""
#endif


#ifndef CONFIG_TCCDIR
#define CONFIG_TCCDIR				"/usr/local/lib/tcc"
#endif


#ifndef CONFIG_LDDIR
#define CONFIG_LDDIR				"lib"
#endif


#ifdef CONFIG_TRIPLET
#define USE_TRIPLET(S)				S "/" CONFIG_TRIPLET
#define ALSO_TRIPLET(s)				USE_TRIPLET(s) ":" S
#else
#define USE_TRIPLET(S)				S
#define ALSO_TRIPLET(S)				S
#endif


// Target-specific CRT.


#define TCC_CRT_STEM				"crt" // The CRT file stem.
#define TCC_CRT_EXTN				".a" // The CRT file extension.


// CRT architecture.


#define TCC_CRT_ARCH_X86			".x86"
#define TCC_CRT_ARCH_ARM			".arm"
#define TCC_CRT_ARCH_000			""


#if defined(TCC_TARGET_X86_64) || defined(TCC_TARGET_I386) 
#define TCC_CRT_ARCH				TCC_CRT_ARCH_X86
#elif defined(TCC_TARGET_ARM64) || defined(TCC_TARGET_ARM)
#define TCC_CRT_ARCH				TCC_CRT_ARCH_ARM
#else
#define TCC_CRT_ARCH				TCC_CRT_ARCH_000
#endif


// CRT bits.


#define TCC_CRT_BITS_64				".64"
#define TCC_CRT_BITS_32				".32"
#define TCC_CRT_BITS_00				""


#if defined(TCC_TARGET_X86_64) || defined(TCC_TARGET_ARM64)
#define TCC_CRT_BITS				TCC_CRT_BITS_64
#elif defined(TCC_TARGET_I386) || defined(TCC_TARGET_ARM)
#define TCC_CRT_BITS				TCC_CRT_BITS_32
#else
#define TCC_CRT_BITS				TCC_CRT_BITS_00
#endif


// CRT OS.


#define TCC_CRT_OSYS_WIN			".w"
#define TCC_CRT_OSYS_LIN			".l"
#define TCC_CRT_OSYS_UNX			".u"
#define TCC_CRT_OSYS_000			""


#if defined(TCC_TARGET_WINDOWS) 
#define TCC_CRT_OSYS				TCC_CRT_OSYS_WIN
#elif defined(TCC_TARGET_LINUX)
#define TCC_CRT_OSYS				TCC_CRT_OSYS_LIN
#elif defined(TCC_TARGET_UNIX)
#define TCC_CRT_OSYS				TCC_CRT_OSYS_UNX
#else
#define TCC_CRT_OSYS				TCC_CRT_OSYS_000
#endif


// Default CRT.


#define TCC_LIBTCC1					TCC_CRT_STEM TCC_CRT_ARCH TCC_CRT_BITS TCC_CRT_OSYS TCC_CRT_EXTN


// Specific CRTs.


#define TCC_CRT_X86_32_WIN			TCC_CRT_STEM TCC_CRT_ARCH_X86 TCC_CRT_BITS_32 TCC_CRT_OSYS_WIN TCC_CRT_EXTN
#define TCC_CRT_X86_64_WIN			TCC_CRT_STEM TCC_CRT_ARCH_X86 TCC_CRT_BITS_64 TCC_CRT_OSYS_WIN TCC_CRT_EXTN
#define TCC_CRT_ARM_32_WIN			TCC_CRT_STEM TCC_CRT_ARCH_ARM TCC_CRT_BITS_32 TCC_CRT_OSYS_WIN TCC_CRT_EXTN
#define TCC_CRT_ARM_64_WIN			TCC_CRT_STEM TCC_CRT_ARCH_ARM TCC_CRT_BITS_64 TCC_CRT_OSYS_WIN TCC_CRT_EXTN


#define TCC_CRT_X86_32_LIN			TCC_CRT_STEM TCC_CRT_ARCH_X86 TCC_CRT_BITS_32 TCC_CRT_OSYS_LIN TCC_CRT_EXTN
#define TCC_CRT_X86_64_LIN			TCC_CRT_STEM TCC_CRT_ARCH_X86 TCC_CRT_BITS_64 TCC_CRT_OSYS_LIN TCC_CRT_EXTN
#define TCC_CRT_ARM_32_LIN			TCC_CRT_STEM TCC_CRT_ARCH_ARM TCC_CRT_BITS_32 TCC_CRT_OSYS_LIN TCC_CRT_EXTN
#define TCC_CRT_ARM_64_LIN			TCC_CRT_STEM TCC_CRT_ARCH_ARM TCC_CRT_BITS_64 TCC_CRT_OSYS_LIN TCC_CRT_EXTN


#define TCC_CRT_X86_32_UNX			TCC_CRT_STEM TCC_CRT_ARCH_X86 TCC_CRT_BITS_32 TCC_CRT_OSYS_UNX TCC_CRT_EXTN
#define TCC_CRT_X86_64_UNX			TCC_CRT_STEM TCC_CRT_ARCH_X86 TCC_CRT_BITS_64 TCC_CRT_OSYS_UNX TCC_CRT_EXTN
#define TCC_CRT_ARM_32_UNX			TCC_CRT_STEM TCC_CRT_ARCH_ARM TCC_CRT_BITS_32 TCC_CRT_OSYS_UNX TCC_CRT_EXTN
#define TCC_CRT_ARM_64_UNX			TCC_CRT_STEM TCC_CRT_ARCH_ARM TCC_CRT_BITS_64 TCC_CRT_OSYS_UNX TCC_CRT_EXTN


// Library to use with CONFIG_USE_LIBGCC instead of default CRT.


#if defined CONFIG_USE_LIBGCC && !defined TCC_LIBGCC
#define TCC_LIBGCC					USE_TRIPLET(CONFIG_SYSROOT "/" CONFIG_LDDIR) "/libgcc_s.so.1"
#endif


// Path to find CRT.


#ifndef CONFIG_TCC_CRTPREFIX
#define CONFIG_TCC_CRTPREFIX		USE_TRIPLET(CONFIG_SYSROOT "/usr/" CONFIG_LDDIR)
#endif


// System include paths.


#ifndef CONFIG_TCC_SYSINCLUDEPATHS	// {B} is substituted by CONFIG_TCCDIR (rsp. -B option).
#ifdef TCC_TARGET_PE
#define								CONFIG_TCC_SYSINCLUDEPATHS "{B}/include" PATHSEP "{B}/include/winapi"
#else
#define CONFIG_TCC_SYSINCLUDEPATHS	"{B}/include" ":" ALSO_TRIPLET(CONFIG_SYSROOT "/usr/local/include") \
    ":" ALSO_TRIPLET(CONFIG_SYSROOT "/usr/include")
#endif
#endif


// Library search paths.


#ifndef CONFIG_TCC_LIBPATHS			// {B} is substituted by CONFIG_TCCDIR (rsp. -B option).
#ifdef TCC_TARGET_PE
#define CONFIG_TCC_LIBPATHS			"{B}/lib"
#else
#define CONFIG_TCC_LIBPATHS			ALSO_TRIPLET(CONFIG_SYSROOT "/usr/" CONFIG_LDDIR) \
	":" ALSO_TRIPLET(CONFIG_SYSROOT "/" CONFIG_LDDIR) \
	":" ALSO_TRIPLET(CONFIG_SYSROOT "/usr/local/" CONFIG_LDDIR)
#endif
#endif


#ifdef _WIN32
#define IS_DIRSEP(c)				(c == '/' || c == '\\')
#define IS_ABSPATH(p)				(IS_DIRSEP(p[0]) || (p[0] && p[1] == ':' && IS_DIRSEP(p[2])))
#define PATHCMP						stricmp
#define PATHSEP						";"
#else
#define IS_DIRSEP(c)				(c == '/')
#define IS_ABSPATH(p)				IS_DIRSEP(p[0])
#define PATHCMP						strcmp
#define PATHSEP						":"
#endif


// Name of ELF interpreter.


#ifndef CONFIG_TCC_ELFINTERP


#if defined __FreeBSD__

#define CONFIG_TCC_ELFINTERP		"/libexec/ld-elf.so.1"

#elif defined __FreeBSD_kernel__

#if defined(TCC_TARGET_X86_64)
#define CONFIG_TCC_ELFINTERP		"/lib/ld-kfreebsd-x86-64.so.1"
#else
#define CONFIG_TCC_ELFINTERP		"/lib/ld.so.1"
#endif

#elif defined __DragonFly__

#define CONFIG_TCC_ELFINTERP		"/usr/libexec/ld-elf.so.2"

#elif defined __NetBSD__

#define CONFIG_TCC_ELFINTERP		"/usr/libexec/ld.elf_so"

#elif defined __GNU__

#define CONFIG_TCC_ELFINTERP		"/lib/ld.so"

#elif defined(TCC_TARGET_PE)

#define CONFIG_TCC_ELFINTERP		"-"

#elif defined(TCC_UCLIBC)

#define CONFIG_TCC_ELFINTERP		"/lib/ld-uClibc.so.0"

#elif defined TCC_TARGET_ARM64

#if defined(TCC_MUSL)
#define CONFIG_TCC_ELFINTERP		"/lib/ld-musl-aarch64.so.1"
#else
#define CONFIG_TCC_ELFINTERP		"/lib/ld-linux-aarch64.so.1"
#endif

#elif defined(TCC_TARGET_X86_64)

#if defined(TCC_MUSL)
#define CONFIG_TCC_ELFINTERP		"/lib/ld-musl-x86_64.so.1"
#else
#define CONFIG_TCC_ELFINTERP		"/lib64/ld-linux-x86-64.so.2"
#endif

#elif !defined(TCC_ARM_EABI)

#if defined(TCC_MUSL)
#define CONFIG_TCC_ELFINTERP		"/lib/ld-musl-arm.so.1"
#else
#define CONFIG_TCC_ELFINTERP		"/lib/ld-linux.so.2"
#endif

#endif


#endif


// Compiler / assembler / linker debugging.


//#define PARSE_DEBUG				1 // Parser debug.
//#define PP_DEBUG					1 // Preprocessor debug.
//#define INC_DEBUG					1 // Include file debug.
//#define MEM_DEBUG					1 // Memory leak debug.
//#define ASM_DEBUG					1 // Assembler debug.


#endif // INCLUDE_CFG_H


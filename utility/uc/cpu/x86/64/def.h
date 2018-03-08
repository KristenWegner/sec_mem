// def.h - Definitions for Intel/AMD x86-64 code generation.


#ifndef INCLUDE_DEF_H
#define INCLUDE_DEF_H		1


#ifndef CONFIG_TCC_ASM
#define CONFIG_TCC_ASM		1
#endif


#ifndef TCC_TARGET_X86_64
#define TCC_TARGET_X86_64	1
#endif


// Number of available registers.


#define NB_REGS				25
#define NB_ASM_REGS			16


// A register can belong to several classes. The classes must be sorted from more general 
// to more precise (see gv2() code which does assumptions on it).


#define RC_INT				0x00001 // Generic integer register.
#define RC_FLOAT			0x00002 // Generic float register.
#define RC_RAX				0x00004
#define RC_RCX				0x00008
#define RC_RDX				0x00010
#define RC_ST0				0x00080 // Only for long double.
#define RC_R8				0x00100
#define RC_R9				0x00200
#define RC_R10				0x00400
#define RC_R11				0x00800
#define RC_XMM0				0x01000
#define RC_XMM1				0x02000
#define RC_XMM2				0x04000
#define RC_XMM3				0x08000
#define RC_XMM4				0x10000
#define RC_XMM5				0x20000
#define RC_XMM6				0x40000
#define RC_XMM7				0x80000
#define RC_IRET				RC_RAX // Function return: integer register.
#define RC_LRET				RC_RDX // Function return: second integer register.
#define RC_FRET				RC_XMM0 // Function return: float register.
#define RC_QRET				RC_XMM1 // Function return: second float register.


// Friendly names for the registers.


enum 
{
	TREG_RAX = 0,
	TREG_RCX = 1,
	TREG_RDX = 2,
	TREG_RSP = 4,
	TREG_RSI = 6,
	TREG_RDI = 7,
	TREG_R8 = 8,
	TREG_R9 = 9,
	TREG_R10 = 10,
	TREG_R11 = 11,
	TREG_XMM0 = 16,
	TREG_XMM1 = 17,
	TREG_XMM2 = 18,
	TREG_XMM3 = 19,
	TREG_XMM4 = 20,
	TREG_XMM5 = 21,
	TREG_XMM6 = 22,
	TREG_XMM7 = 23,
	TREG_ST0 = 24,
	TREG_MEM = 0x20
};


#define REX_BASE(R)			(((R) >> 3) & 1)
#define REG_VALUE(R)		((R) & 7)


// Return registers for functions.


#define REG_IRET			TREG_RAX // Single word int return register.
#define REG_LRET			TREG_RDX // Second word return register (for long long).
#define REG_FRET			TREG_XMM0 // Float return register.
#define REG_QRET			TREG_XMM1 // Second float return register.


// Defined if function parameters must be evaluated in reverse order.


#define INVERT_FUNC_PARAMS	1


// Pointer size, in bytes.


#define PTR_SIZE			8


// Long double size and alignment, in bytes.


#define LDOUBLE_SIZE		16
#define LDOUBLE_ALIGN		16


// Maximum alignment (for aligned attribute support).


#define MAX_ALIGN			16


#define EM_TCC_TARGET		EM_X86_64


// Relocation.


#define R_DATA_32			R_X86_64_32S
#define R_DATA_PTR			R_X86_64_64
#define R_JMP_SLOT			R_X86_64_JUMP_SLOT
#define R_GLOB_DAT			R_X86_64_GLOB_DAT
#define R_COPY				R_X86_64_COPY
#define R_RELATIVE			R_X86_64_RELATIVE
#define R_NUM				 R_X86_64_NUM


#define ELF_START_ADDR		0x400000
#define ELF_PAGE_SIZE		0x200000


#define PCRELATIVE_DLLPLT	1
#define RELOCATE_DLLPLT		1


#endif // INCLUDE_DEF_H


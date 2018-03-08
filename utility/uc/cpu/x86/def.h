// def.h - Definitions for Intel x86/i386 code generation.


#ifndef INCLUDE_DEF_H
#define INCLUDE_DEF_H		1


#ifndef TCC_TARGET_I386
#define TCC_TARGET_I386		1
#endif


#ifndef CONFIG_TCC_ASM
#define CONFIG_TCC_ASM		1
#endif


// Number of available registers.


#define NB_REGS				5
#define NB_ASM_REGS			8


// A register can belong to several classes. The classes must be sorted from more general 
// to more precise (see gv2() code which does assumptions on it).


#define RC_INT				0x0001 // Generic integer register.
#define RC_FLOAT			0x0002 // Generic float register.
#define RC_EAX				0x0004
#define RC_ST0				0x0008 
#define RC_ECX				0x0010
#define RC_EDX				0x0020
#define RC_EBX				0x0040
#define RC_IRET				RC_EAX // Function return: integer register.
#define RC_LRET				RC_EDX // Function return: second integer register.
#define RC_FRET				RC_ST0 // Function return: float register.


// Friendly names for the registers.


enum 
{
	TREG_EAX = 0,
	TREG_ECX,
	TREG_EDX,
	TREG_EBX,
	TREG_ST0,
	TREG_ESP = 4
};


// Return registers for functions.


#define REG_IRET			TREG_EAX // Single word int return register.
#define REG_LRET			TREG_EDX // Second word return register (for long long).
#define REG_FRET			TREG_ST0 // Float return register.


// Defined if function parameters must be evaluated in reverse order.


#define INVERT_FUNC_PARAMS	1

// Defined if structures are passed as pointers. Otherwise structures are directly 
// pushed on stack.


#undef FUNC_STRUCT_PARAM_AS_PTR


// Pointer size, in bytes.


#define PTR_SIZE			4

// Long double size and alignment, in bytes.


#define LDOUBLE_SIZE		12
#define LDOUBLE_ALIGN		4


// Maximum alignment (for aligned attribute support).


#define MAX_ALIGN			8


#define EM_TCC_TARGET		EM_386


// Relocation for 32 bit data.


#define R_DATA_32			R_386_32
#define R_DATA_PTR			R_386_32
#define R_JMP_SLOT			R_386_JMP_SLOT
#define R_GLOB_DAT			R_386_GLOB_DAT
#define R_COPY				R_386_COPY
#define R_RELATIVE			R_386_RELATIVE
#define R_NUM				R_386_NUM


#define ELF_START_ADDR		0x08048000
#define ELF_PAGE_SIZE		0x1000


#define PCRELATIVE_DLLPLT	0
#define RELOCATE_DLLPLT		0


#endif // INCLUDE_DEF_H


// def.h - Definitions for ARMv4 code generation.


#ifndef INCLUDE_DEF_H
#define INCLUDE_DEF_H		1


#ifndef CONFIG_TCC_ASM
#define CONFIG_TCC_ASM		1
#endif


#ifndef TCC_TARGET_ARM
#define TCC_TARGET_ARM		1
#endif


#if defined(TCC_ARM_EABI) && !defined(TCC_ARM_VFP)
#error "This implementation currently only supports ARM EABI float computation with VFP instructions."
#endif


// Number of available registers.


#ifdef TCC_ARM_VFP
#define NB_REGS				13
#else
#define NB_REGS				9
#endif


#ifndef TCC_CPU_VERSION
#define TCC_CPU_VERSION		5
#endif


// A register can belong to several classes. The classes must be sorted from more general 
// to more precise (see gv2() code which does assumptions on it).


#define RC_INT				0x0001 // Generic integer register.
#define RC_FLOAT			0x0002 // Generic float register.
#define RC_R0				0x0004
#define RC_R1				0x0008
#define RC_R2				0x0010
#define RC_R3				0x0020
#define RC_R12				0x0040
#define RC_F0				0x0080
#define RC_F1				0x0100
#define RC_F2				0x0200
#define RC_F3				0x0400


#ifdef TCC_ARM_VFP
#define RC_F4				0x0800
#define RC_F5				0x1000
#define RC_F6				0x2000
#define RC_F7				0x4000
#endif


#define RC_IRET				RC_R0 // Function return: integer register.
#define RC_LRET				RC_R1 // Function return: second integer register.
#define RC_FRET				RC_F0 // Function return: float register.


// Friendly names for the registers.


enum 
{
	TREG_R0 = 0,
	TREG_R1,
	TREG_R2,
	TREG_R3,
	TREG_R12,
	TREG_F0,
	TREG_F1,
	TREG_F2,
	TREG_F3,

#ifdef TCC_ARM_VFP
	TREG_F4,
	TREG_F5,
	TREG_F6,
	TREG_F7,
#endif

	TREG_SP = 13,
	TREG_LR,
};


#ifdef TCC_ARM_VFP
#define T2CPR(V)			(((V) & VT_BTYPE) != VT_FLOAT ? 0x100 : 0)
#endif


// Return registers for functions.


#define REG_IRET			TREG_R0 // Single word int return register.
#define REG_LRET			TREG_R1 // Second word return register (for long long).
#define REG_FRET			TREG_F0 // Float return register.


#ifdef TCC_ARM_EABI
#define TOK___divdi3		TOK___aeabi_ldivmod
#define TOK___moddi3		TOK___aeabi_ldivmod
#define TOK___udivdi3		TOK___aeabi_uldivmod
#define TOK___umoddi3		TOK___aeabi_uldivmod
#define TOK_memcpy			TOK___aeabi_memcpy
#define TOK_memmove			TOK___aeabi_memmove
#define TOK_memset			TOK___aeabi_memset
#endif


// Defined if function parameters must be evaluated in reverse order.


#define INVERT_FUNC_PARAMS	1


// Defined if structures are passed as pointers. Otherwise structures are directly 
// pushed on stack.


#undef FUNC_STRUCT_PARAM_AS_PTR


// Pointer size, in bytes.


#define PTR_SIZE			4

// Long double size and alignment, in bytes.


#ifdef TCC_ARM_VFP
#define LDOUBLE_SIZE		8
#endif


#ifndef LDOUBLE_SIZE
#define LDOUBLE_SIZE		8
#endif


#ifdef TCC_ARM_EABI
#define LDOUBLE_ALIGN		8
#else
#define LDOUBLE_ALIGN		4
#endif


// Maximum alignment (for aligned attribute support).


#define MAX_ALIGN			8
#define CHAR_IS_UNSIGNED	1


#define EM_TCC_TARGET		EM_ARM


// Relocation type for 32-bit data relocation.


#define R_DATA_32			R_ARM_ABS32
#define R_DATA_PTR			R_ARM_ABS32
#define R_JMP_SLOT			R_ARM_JUMP_SLOT
#define R_GLOB_DAT			R_ARM_GLOB_DAT
#define R_COPY				R_ARM_COPY
#define R_RELATIVE			R_ARM_RELATIVE
#define R_NUM				R_ARM_NUM


#define ELF_START_ADDR		0x00008000
#define ELF_PAGE_SIZE		0x1000


#define PCRELATIVE_DLLPLT	1
#define RELOCATE_DLLPLT		0


enum float_abi
{
	ARM_SOFTFP_FLOAT,
	ARM_HARD_FLOAT,
};


#endif // INCLUDE_DEF_H


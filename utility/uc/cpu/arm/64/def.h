// def.h - Definitions for ARMv8 (64-bit) code generation.


#ifndef INCLUDE_DEF_H
#define INCLUDE_DEF_H		1


#ifndef CONFIG_TCC_ASM
#define CONFIG_TCC_ASM		1
#endif


#ifndef TCC_TARGET_ARM64
#define TCC_TARGET_ARM64	1
#endif


// Number of registers available to allocator.


#define NB_REGS				28 // x0 - x18, x30, v0 - v7.


#define TREG_R(X)			(X) // X = 0 .. 18.
#define TREG_R30			19
#define TREG_F(X)			(X + 20) // X = 0 .. 7.


// Register classes sorted from more general to more precise.


#define RC_INT				(1 << 0)
#define RC_FLOAT			(1 << 1)
#define RC_R(X)				(1 << (2 + (X))) // X = 0 .. 18.
#define RC_R30				(1 << 21)
#define RC_F(X)				(1 << (22 + (X))) // X = 0 .. 7.


#define RC_IRET				(RC_R(0)) // Integer return register class.
#define RC_FRET				(RC_F(0)) // Float return register class.


// Specific registers.


#define REG_IRET			(TREG_R(0)) // Integer return register number.
#define REG_FRET			(TREG_F(0)) // Float return register number.


// General.


#define PTR_SIZE			8
#define LDOUBLE_SIZE		16
#define LDOUBLE_ALIGN		16
#define MAX_ALIGN			16
#define CHAR_IS_UNSIGNED	1


#define EM_TCC_TARGET		EM_AARCH64


// Relocations.


#define R_DATA_32			R_AARCH64_ABS32
#define R_DATA_PTR			R_AARCH64_ABS64
#define R_JMP_SLOT			R_AARCH64_JUMP_SLOT
#define R_GLOB_DAT			R_AARCH64_GLOB_DAT
#define R_COPY				R_AARCH64_COPY
#define R_RELATIVE			R_AARCH64_RELATIVE
#define R_NUM				R_AARCH64_NUM


#define ELF_START_ADDR		0x00400000
#define ELF_PAGE_SIZE		0x1000


#define PCRELATIVE_DLLPLT	1
#define RELOCATE_DLLPLT		1


#endif // INCLUDE_DEF_H


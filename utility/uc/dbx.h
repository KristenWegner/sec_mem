// dbx.h - GNU DBX symbol codes.


/*
Table of DBX symbol codes for the GNU system.

Copyright (C) 1988, 1997 Free Software Foundation, Inc. This file is part of 
the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or modify it 
under the terms of the GNU Library General Public License as published by the 
Free Software Foundation; either version 2 of the License, or (at your option) 
any later version.

The GNU C Library is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
FITNESS FOR A PARTICULAR PURPOSE. See the GNU Library General Public License 
for more details.

You should have received a copy of the GNU Library General Public License 
along with the GNU C Library; see the file COPYING.LIB. If not, write to the 
Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
02111-1307, USA.

This contains contribution from Cygnus Support.
*/


#ifndef INCLUDE_DBX_H
#define INCLUDE_DBX_H 1


// Indicate the GNU 'stab' is in use.
#define __GNU_STAB__ 1


enum __stab_debug_code
{
#define __define_stab(NAME, CODE, STRING) NAME = CODE,

	/* Global variable.  Only the name is significant.
	To find the address, look in the corresponding external symbol.  */
	__define_stab(N_GSYM, 0x20, "GSYM")

	/* Function name for BSD Fortran.  Only the name is significant.
	To find the address, look in the corresponding external symbol.  */
	__define_stab(N_FNAME, 0x22, "FNAME")

	/* Function name or text-segment variable for C.  Value is its address.
	Desc is supposedly starting line number, but GCC doesn't set it
	and DBX seems not to miss it.  */
	__define_stab(N_FUN, 0x24, "FUN")

	/* Data-segment variable with internal linkage.  Value is its address.
	"Static uc_symbol_t".  */
	__define_stab(N_STSYM, 0x26, "STSYM")

	/* BSS-segment variable with internal linkage.  Value is its address.  */
	__define_stab(N_LCSYM, 0x28, "LCSYM")

	/* Name of main routine.  Only the name is significant.
	This is not used in C.  */
	__define_stab(N_MAIN, 0x2a, "MAIN")

	/* Global symbol in Pascal.
	Supposedly the value is its line number; I'm skeptical.  */
	__define_stab(N_PC, 0x30, "PC")

	/* Number of symbols:  0, files,,funcs,lines according to Ultrix V4.0. */
	__define_stab(N_NSYMS, 0x32, "NSYMS")

	/* "No DST map for sym: name, ,0,type,ignored"  according to Ultrix V4.0. */
	__define_stab(N_NOMAP, 0x34, "NOMAP")

	/* New stab from Solaris.  I don't know what it means, but it
	don't seem to contain useful information.  */
	__define_stab(N_OBJ, 0x38, "OBJ")

	/* New stab from Solaris.  I don't know what it means, but it
	don't seem to contain useful information.  Possibly related to the
	optimization flags used in this module.  */
	__define_stab(N_OPT, 0x3c, "OPT")

	/* Register variable.  Value is number of register.  */
	__define_stab(N_RSYM, 0x40, "RSYM")

	/* Modula-2 compilation unit.  Can someone say what info it contains?  */
	__define_stab(N_M2C, 0x42, "M2C")

	/* Line number in text segment.  Desc is the line number;
	value is corresponding address.  */
	__define_stab(N_SLINE, 0x44, "SLINE")

	/* Similar, for data segment.  */
	__define_stab(N_DSLINE, 0x46, "DSLINE")

	/* Similar, for bss segment.  */
	__define_stab(N_BSLINE, 0x48, "BSLINE")

	/* Sun's source-code browser stabs.  ?? Don't know what the fields are.
	Supposedly the field is "path to associated .cb file".  THIS VALUE
	OVERLAPS WITH N_BSLINE!  */
	__define_stab(N_BROWS, 0x48, "BROWS")

	/* GNU Modula-2 definition module dependency.  Value is the modification time
	of the definition file.  Other is non-zero if it is imported with the
	GNU M2 keyword %INITIALIZE.  Perhaps N_M2C can be used if there
	are enough empty fields? */
	__define_stab(N_DEFD, 0x4a, "DEFD")

	/* THE FOLLOWING TWO STAB VALUES CONFLICT.  Happily, one is for Modula-2
	and one is for C++.   Still,... */
	/* GNU C++ exception variable.  Name is variable name.  */
	__define_stab(N_EHDECL, 0x50, "EHDECL")

	/* Modula2 info "for imc":  name,,0,0,0  according to Ultrix V4.0.  */
	__define_stab(N_MOD2, 0x50, "MOD2")

	/* GNU C++ `catch' clause.  Value is its address.  Desc is nonzero if
	this entry is immediately followed by a CAUGHT stab saying what exception
	was caught.  Multiple CAUGHT stabs means that multiple exceptions
	can be caught here.  If Desc is 0, it means all exceptions are caught
	here.  */
	__define_stab(N_CATCH, 0x54, "CATCH")

	/* Structure or union element.  Value is offset in the structure.  */
	__define_stab(N_SSYM, 0x60, "SSYM")

	/* Name of main source file.
	Value is starting text address of the compilation.  */
	__define_stab(N_SO, 0x64, "SO")

	/* Automatic variable in the stack.  Value is offset from frame pointer.
	Also used for type descriptions.  */
	__define_stab(N_LSYM, 0x80, "LSYM")

	/* Beginning of an include file.  Only Sun uses this.
	In an object file, only the name is significant.
	The Sun linker puts data into some of the other fields.  */
	__define_stab(N_BINCL, 0x82, "BINCL")

	/* Name of sub-source file (#include file).
	Value is starting text address of the compilation.  */
	__define_stab(N_SOL, 0x84, "SOL")

	/* Parameter variable.  Value is offset from argument pointer.
	(On most machines the argument pointer is the same as the frame pointer.  */
	__define_stab(N_PSYM, 0xa0, "PSYM")

	/* End of an include file.  No name.
	This and N_BINCL act as brackets around the file's output.
	In an object file, there is no significant data in this entry.
	The Sun linker puts data into some of the fields.  */
	__define_stab(N_EINCL, 0xa2, "EINCL")

	/* Alternate entry point.  Value is its address.  */
	__define_stab(N_ENTRY, 0xa4, "ENTRY")

	/* Beginning of lexical block.
	The desc is the nesting level in lexical blocks.
	The value is the address of the start of the text for the block.
	The variables declared inside the block *precede* the N_LBRAC symbol.  */
	__define_stab(N_LBRAC, 0xc0, "LBRAC")

	/* Place holder for deleted include file.  Replaces a N_BINCL and everything
	up to the corresponding N_EINCL. */
	__define_stab(N_EXCL, 0xc2, "EXCL")

	/* Modula-2 scope information. */
	__define_stab(N_SCOPE, 0xc4, "SCOPE")

	/* End of a lexical block.  Desc matches the N_LBRAC's desc.
	The value is the address of the end of the text for the block.  */
	__define_stab(N_RBRAC, 0xe0, "RBRAC")

	/* Begin named common block.  Only the name is significant. */
	__define_stab(N_BCOMM, 0xe2, "BCOMM")

	/* End named common block. Only the name is significant (and it should match the N_BCOMM). */
	__define_stab(N_ECOMM, 0xe4, "ECOMM")

	/* End common (local name): value is address. */
	__define_stab(N_ECOML, 0xe8, "ECOML")

	/* STAB's are used on Gould systems for Non-Base register symbols. */
	__define_stab(N_NBTEXT, 0xF0, "NBTEXT")
	__define_stab(N_NBDATA, 0xF2, "NBDATA")
	__define_stab(N_NBBSS, 0xF4, "NBBSS")
	__define_stab(N_NBSTS, 0xF6, "NBSTS")
	__define_stab(N_NBLCS, 0xF8, "NBLCS")

	/* Second symbol entry containing a length-value for the preceding entry.
	The value is the length.  */
	__define_stab(N_LENG, 0xfe, "LENG")

#undef __define_stab

	LAST_UNUSED_STAB_CODE
};


#endif // INCLUDE_DBX_H


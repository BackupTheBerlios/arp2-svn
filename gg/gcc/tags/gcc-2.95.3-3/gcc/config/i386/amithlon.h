/* Definitions for Intel 386 running Linux-based GNU systems with ELF format.
   Copyright (C) 1994, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   Contributed by Eric Youngdale.
   Modified for stabs-in-ELF by H.J. Lu.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Amithlon is basically just a Linux process, so lets act as one... */

#include <i386/linux.h>

/* Now undo and redefine */
#undef LINUX_DEFAULT_ELF

#undef TARGET_VERSION
#define TARGET_VERSION fprintf (stderr, " (i386be Amithlon/ELF)");

/* Some predicates used in i386be.md */
#undef PREDICATE_CODES
#define PREDICATE_CODES						\
  {"cint_operand",        {CONST_INT}},				\
  {"reg_or_cint_operand", {SUBREG, REG, CONST_INT}}, 		\

/* Now the fun part: let's use big endian integers, but little endian
   floats and instructions ... */

/* Define this if most significant byte of a word is the lowest numbered.  */
#undef BYTES_BIG_ENDIAN
#define BYTES_BIG_ENDIAN 1

/* Define this if most significant word of a multiword number is the lowest
   numbered.  */
#undef WORDS_BIG_ENDIAN
#define WORDS_BIG_ENDIAN 1

/* Define this if most significant word of doubles is the lowest numbered. */
#undef FLOAT_WORDS_BIG_ENDIAN
#define FLOAT_WORDS_BIG_ENDIAN 0










#if 0

/* Since GCC assumes the endian word order for registers is the same as for
   memory, we have to swap eax and edx :( */

This turned out to be WAY to difficult ....


/* 1 for registers that have pervasive standard uses
   and are not available for the register allocator.
   On the 80386, the stack pointer is such, as is the arg pointer. */

#undef FIXED_REGISTERS
#define FIXED_REGISTERS \
/*dx,ax,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7,arg*/       \
{  0, 0, 0, 0, 0, 0, 0, 1, 0,  0,  0,  0,  0,  0,  0,  0,  1 }

/* 1 for registers not available across function calls.
   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like.  */

#undef CALL_USED_REGISTERS
#define CALL_USED_REGISTERS \
/*dx,ax,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7,arg*/ \
{  1, 1, 1, 0, 0, 0, 0, 1, 1,  1,  1,  1,  1,  1,  1,  1,  1 }

/* Order in which to allocate registers.  Each register must be
   listed once, even those in FIXED_REGISTERS.  List frame pointer
   late and fixed registers last.  Note that, in general, we prefer
   registers listed in CALL_USED_REGISTERS, keeping the others
   available for storage of persistent values.

   Three different versions of REG_ALLOC_ORDER have been tried:

   If the order is edx, ecx, eax, ... it produces a slightly faster compiler,
   but slower code on simple functions returning values in eax.

   If the order is eax, ecx, edx, ... it causes reload to abort when compiling
   perl 4.036 due to not being able to create a DImode register (to hold a 2
   word union).

   If the order is eax, edx, ecx, ... it produces better code for simple
   functions, and a slightly slower compiler.  Users complained about the code
   generated by allocating edx first, so restore the 'natural' order of things. */

#undef REG_ALLOC_ORDER
#define REG_ALLOC_ORDER \
/*ax,dx,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7,arg*/ \
{  1, 0, 2, 3, 4, 5, 6, 7, 8,  9, 10, 11, 12, 13, 14, 15, 16 }

/* How to refer to registers in assembler output.
   This sequence is indexed by compiler's hard-register-number (see above). */

/* In order to refer to the first 8 regs as 32 bit regs prefix an "e"
   For non floating point regs, the following are the HImode names.

   For float regs, the stack top is sometimes referred to as "%st(0)"
   instead of just "%st".  PRINT_REG handles this with the "y" code.  */

#undef HI_REGISTER_NAMES
#define HI_REGISTER_NAMES \
{"dx","ax","cx","bx","si","di","bp","sp",          \
 "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)","" }

#define REGISTER_NAMES HI_REGISTER_NAMES

/* Table of additional register names to use in user input.  */

#undef ADDITIONAL_REGISTER_NAMES
#define ADDITIONAL_REGISTER_NAMES \
{ { "edx", 0 }, { "eax", 1 }, { "ecx", 2 }, { "ebx", 3 },	\
  { "esi", 4 }, { "edi", 5 }, { "ebp", 6 }, { "esp", 7 },	\
  { "al", 0 }, { "dl", 1 }, { "cl", 2 }, { "bl", 3 },		\
  { "ah", 0 }, { "dh", 1 }, { "ch", 2 }, { "bh", 3 } }

/* Note we are omitting these since currently I don't know how
to get gcc to use these, since they want the same but different
number as al, and ax.
*/

/* note the last four are not really qi_registers, but
   the md will have to never output movb into one of them
   only a movw .  There is no movb into the last four regs */

#undef QI_REGISTER_NAMES
#define QI_REGISTER_NAMES \
{"dl", "al", "cl", "bl", "si", "di", "bp", "sp",}

/* These parallel the array above, and can be used to access bits 8:15
   of regs 0 through 3. */

#undef QI_HIGH_REGISTER_NAMES
#define QI_HIGH_REGISTER_NAMES \
{"dh", "ah", "ch", "bh", }

/* How to renumber registers for dbx and gdb.  */

/* {1,2,0,3,6,7,4,5,12,13,14,15,16,17}  */
#undef DBX_REGISTER_NUMBER
#define DBX_REGISTER_NUMBER(n) \
((n) == 0 ? 1 : \
 (n) == 1 ? 2 : \
 (n) == 2 ? 0 : \
 (n) == 3 ? 3 : \
 (n) == 4 ? 6 : \
 (n) == 5 ? 7 : \
 (n) == 6 ? 4 : \
 (n) == 7 ? 5 : \
 (n) + 4)

#endif  // 0







/* The assembler will use big endian for data section, but we must
 * swap the words in 64 bit integers ourself. We also have to "undo"
 * the byteswap in the individual words for floats
 */

/* This is how to output an assembler line defining an `long long'
   constant.  */

#undef ASM_OUTPUT_DOUBLE_INT
#define ASM_OUTPUT_DOUBLE_INT(FILE,VALUE)				\
do {									\
      assemble_integer (operand_subword ((VALUE), 0, 0, DImode),	\
                        UNITS_PER_WORD, 1);				\
      assemble_integer (operand_subword ((VALUE), 1, 0, DImode),	\
                        UNITS_PER_WORD, 1);				\
} while (0)


/* This is how to output an assembler line defining a `double' constant.  */

#undef ASM_OUTPUT_DOUBLE
#define ASM_OUTPUT_DOUBLE(FILE,VALUE)					\
do { long l[2];								\
     REAL_VALUE_TO_TARGET_DOUBLE (VALUE, l);				\
     fprintf (FILE, "%s 0x%lx,0x%lx\n", ASM_LONG, bswap_32(l[0]), bswap_32(l[1]));		\
   } while (0)

/* This is how to output a `long double' extended real constant. */

#undef ASM_OUTPUT_LONG_DOUBLE
#define ASM_OUTPUT_LONG_DOUBLE(FILE,VALUE)  		\
do { long l[3];						\
     REAL_VALUE_TO_TARGET_LONG_DOUBLE (VALUE, l);	\
     fprintf (FILE, "%s 0x%lx,0x%lx,0x%lx\n", ASM_LONG, bswap_32(l[0]), bswap_32(l[1]), bswap_32(l[2])); \
   } while (0)

/* This is how to output an assembler line defining a `float' constant.  */

#undef ASM_OUTPUT_FLOAT
#define ASM_OUTPUT_FLOAT(FILE,VALUE)			\
do { long l;						\
     REAL_VALUE_TO_TARGET_SINGLE (VALUE, l);		\
     fprintf ((FILE), "%s 0x%lx\n", ASM_LONG, bswap_32(l));	\
   } while (0)


#undef CPP_SPEC
#define CPP_SPEC							\
  "%(cpp_cpu) "								\
  "-D__BIG_ENDIAN__ -Amachine(bigendian) "				\
  "%{!ansi: "								\
   "%{!noixemul:-Dixemul} "						\
   "%{noixemul:-Dlibnix}} "						\
  "%{!noixemul:-D__ixemul__ -D__ixemul} "				\
  "%{noixemul:-D__libnix__ -D__libnix} "				\
  "%{msoft-float: -D_SOFT_FLOAT}"


/* amiga/amigaos are the new "standard" defines for the Amiga.
 * MCH_AMIGA, AMIGA, __chip etc. are used in other compilers and are
 * provided for compatibility reasons.
 * We define both AmigaOS and Amithlon systems. */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES \
  "-D__ELF__ "								\
  "-Damiga -Damigaos -DMCH_AMIGA -DAMIGA "				\
  "-Damithlon "								\
  "-Asystem(amigaos) -Asystem(amithlon) "

#undef CC1_SPEC
#define CC1_SPEC "%(cc1_cpu)"

/* Choose the right startup file, depending on whether we use base relative
   code, base relative code with automatic relocation (-resident), or plain
   crt0.o. 
  
   Profiling is currently only available for plain startup.
   mcrt0.o does not exist, but at least we get an error message when using
   the -p flag */

#undef STARTFILE_SPEC
#define STARTFILE_SPEC							\
  "%{!noixemul: "							\
   "%{pg:gcrt0.o%s} "							\
   "%{!pg: %{p:mcrt0.o%s} "						\
          "%{!p:crt0.o%s}}} "						\
  "%{noixemul:"								\
    "%{resident:libnix/nrcrt0.o%s} "					\
    "%{!resident:%{fbaserel:libnix/nbcrt0.o%s}%{!fbaserel:libnix/ncrt0.o%s}} " \
   "libnix/crti.o%s} "                                                  \
  "%{!shared:crtbegin.o%s} %{shared:crtbeginS.o%s}"

#undef ENDFILE_SPEC
#define ENDFILE_SPEC "%{noixemul:-lstubs} "                             \
                     "%{!shared:crtend.o%s} %{shared:crtendS.o%s} "     \
                     "%{noixemul:libnix/crtn.o%s}"

/* Automatically search libamiga.a for AmigaOS specific functions.  Note
   that we first search the standard C library to resolve as much as
   possible from there, since it has names that are duplicated in libamiga.a
   which we *don't* want from there.  Then search libamiga.a for any calls
   that were not generated inline, and finally search the standard C library
   again to resolve any references that libamiga.a might have generated.
   This may only be a temporary solution since it might be better to simply
   remove the things from libamiga.a that should be pulled in from libc.a
   instead, which would eliminate the first reference to libc.a.  Note that
   if we don't search it automatically, it is very easy for the user to try
   to put in a -lamiga himself and get it in the wrong place, so that (for
   example) calls like sprintf come from -lamiga rather than -lc. */

#undef LIB_SPEC
#define LIB_SPEC							\
  "%{!noixemul: "							\
    "%{!p:%{!pg:-lc -lamiga -lc}} "					\
    "%{p:-lc_p}%{pg:-lc_p}} "						\
  "%{noixemul:-lnixmain -lnix -lamiga "					\
    "%{mstackcheck:-lstack} "						\
    "%{mstackextend:-lstack}} -lamigastubs"

/* Pass appropriate linker flavours depending on user-supplied
   commandline options.  */

#undef LINK_SPEC
#define LINK_SPEC							\
  "%{noixemul:-fl libnix} "						\

/* Get perform_* macros to build libgcc.a.  */
/* Do I have to? #include "i386/perform.h" */


/* begin-GG-local: dynamic libraries */

/* This macro is used to check if all collect2 facilities should be used.
   We need a few special ones, like stripping after linking.  */

#define DO_COLLECTING (do_collecting || amigaos_do_collecting())

/* This macro is called in collect2 for every GCC argument name.
   ARG is a part of commandline (without '\0' at the end).  */

#define COLLECT2_GCC_OPTIONS_HOOK(ARG) amigaos_gccopts_hook(ARG)

/* This macro is called in collect2 for every ld's "-l" or "*.o" or "*.a"
   argument.  ARG is a complete argument, with '\0' at the end.  */

#define COLLECT2_LIBNAME_HOOK(ARG) amigaos_libname_hook(ARG)

/* This macro is called at collect2 exit, to clean everything up.  */

#define COLLECT2_EXTRA_CLEANUP amigaos_collect2_cleanup

/* This macro is called just before the first linker invocation.
   LD1_ARGV is "char** argv", which will be passed to "ld".  STRIP is an
   *address* of "strip_flag" variable.  */

#define COLLECT2_PRELINK_HOOK(LD1_ARGV, STRIP) \
amigaos_prelink_hook((LD1_ARGV), (STRIP))

/* This macro is called just after the first linker invocation, in place of
   "nm" and "ldd".  OUTPUT_FILE is the executable's filename.  */

#define COLLECT2_POSTLINK_HOOK(OUTPUT_FILE) amigaos_postlink_hook(OUTPUT_FILE)
/* end-GG-local */


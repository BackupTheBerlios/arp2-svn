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
#define TARGET_VERSION fprintf (stderr, " (i386be AMIthlon/ELF)");

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
  "%{noixemul:libnix/startup.o%s}"

#undef ENDFILE_SPEC
 
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
    "%{mstackextend:-lstack}}"

/* Pass appropriate linker flavours depending on user-supplied
   commandline options.  */

#undef LINK_SPEC
#define LINK_SPEC							\
  "%{noixemul:-fl libnix} "						\

/* Get perform_* macros to build libgcc.a.  */
/* Do I have to? #include "i386/perform.h" */


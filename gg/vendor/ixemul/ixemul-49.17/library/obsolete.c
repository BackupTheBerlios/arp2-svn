/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Glue for backward compatibility with code compiled by GCC with
 * PCC_STATIC_STRUCT_RETURN defined (i.e., upto and including GCC 2.7.2 Geek Gadgets
 * snapshot 960902).
 */
#ifndef NATIVE_MORPHOS
asm("
.lcomm LF0,8
.text
	.even
.globl  ___obsolete_div
___obsolete_div:
	movel   #LF0,a1
	jmp     _div

.lcomm LF1,8
.globl  ___obsolete_ldiv
___obsolete_ldiv:
	movel   #LF1,a1
	jmp     _ldiv

.lcomm LF2,4
.globl  ___obsolete_inet_makeaddr
___obsolete_inet_makeaddr:
	movel   #LF2,a1
	jmp     _inet_makeaddr
");
#else

#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <exec/types.h>
#include <emul/emulinterface.h>
#include <emul/emulregs.h>

void *_trampoline___obsolete_div(void) {
  GETEMULHANDLE
  int *p = (int *)REG_A7;
  static div_t r;
  r = div(p[1], p[2]);
  return &r;
}

struct EmulLibEntry _gate___obsolete_div = {
  TRAP_LIB, 0, (void(*)())_trampoline___obsolete_div
};

void *_trampoline___obsolete_ldiv(void) {
  GETEMULHANDLE
  int *p = (int *)REG_A7;
  static ldiv_t r;
  r = ldiv(p[1], p[2]);
  return &r;
}

struct EmulLibEntry _gate___obsolete_ldiv = {
  TRAP_LIB, 0, (void(*)())_trampoline___obsolete_ldiv
};

void *_trampoline___obsolete_inet_makeaddr(void) {
  GETEMULHANDLE
  int *p = (int *)REG_A7;
  static struct in_addr r;
  r = inet_makeaddr(p[1], p[2]);
  return &r;
}

struct EmulLibEntry _gate___obsolete_inet_makeaddr = {
  TRAP_LIB, 0, (void(*)())_trampoline___obsolete_inet_makeaddr
};

#endif

/*
 *  This file is part of the ixemul package for the Amiga.
 *  Copyright (C) 1994 Rafael W. Luebbert
 *  Copyright (C) 1997 Hans Verkuil
 *
 *  This source is placed in the public domain.
 */

#if 0
#undef FOR_LIBC // No longer supported since pOS is dead...
#if defined(FOR_LIBC)

#define _KERNEL
#include <ixemul.h>

#ifdef FOR_LIBC

typedef char CHAR;
#include <pInline/pExec2.h>
#include "a4.h"

#undef SysBase
extern struct ExecBase *SysBase;
extern void *gb_ExecLib;
#define gb_ExecBase SysBase
static void pos_kprintf(const char *format, ...)

#else

void KPrintF(const char *format, ...)

#endif
{
  pOS_VKPrintf(format, (ULONG *)(&format + 1));
}
#endif

#if !defined(NATIVE_MORPHOS) && defined(FOR_LIBC)
asm("
	.globl  _KPrintF

KPutChar:
	movel   a6,sp@-
	movel   4:W,a6
	jsr     a6@(-516:W)
	movel   sp@+,a6
	rts

KDoFmt:
	movel   a6,sp@-
	movel   4:W,a6
	jsr     a6@(-522:W)
	movel   sp@+,a6
	rts

_KPrintF:
	movel   "A4(_ix_os)",d0
	cmpil   #0x704F5300,d0
	beqs    _pos_kprintf
	   lea     sp@(4),a1
	movel   a1@+,a0
	movel   a2,sp@-
	lea     KPutChar,a2
	jbsr    KDoFmt
	movel   sp@+,a2
	rts
");

#endif
#endif

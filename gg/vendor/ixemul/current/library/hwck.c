/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1994  Rafael W. Luebbert
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
 *
 *  $Id: hwck.c,v 1.5 2001/06/01 17:40:09 emm Exp $
 *
 *  $Log: hwck.c,v $
 *  Revision 1.5  2001/06/01 17:40:09  emm
 *  Simplified signal handling. Minor old fixes.
 *
 *  Revision 1.4  2000/07/23 19:17:33  emm
 *  Added ppc stack extension support. Improved glues. Fixed some bugs.
 *
 *  Revision 1.3  2000/06/20 22:17:20  emm
 *  First attempt at a native MorphOS ixemul
 *
 *  Revision 1.2  2000/05/18 19:52:07  emm
 *  MorphOS support added. Not fully working.
 *
 *  Revision 1.1.1.1  2000/05/07 19:38:04  emm
 *  Imported sources
 *
 *  Revision 1.2  2000/05/04 19:33:16  nobody
 *  Replaced tc_Launch polling by exceptions
 *
 *  Revision 1.1.1.1  2000/04/29 00:48:50  nobody
 *  Initial import
 *
 * Revision 1.3  1994/10/18  09:12:51  rluebbert
 * Check for math emulation code (881 || 882) in 040s.  Assume no FPU otherwise.
 * No 040 FPU instruction support yet.
 *
 * Revision 1.2  1994/06/22  14:56:04  rluebbert
 * added struct ExecBase **4
 *
 * Revision 1.1  1994/06/19  15:11:39  rluebbert
 * Initial revision
 *
 */

#define _KERNEL
#include <ixemul.h>
#include <stdarg.h>
#include <string.h>
#ifdef __pos__
#include <pIntui/EasyReq.h>
#include <pInline/pIntui2.h>
#define gb_IntuiBase IntuitionBase      /* reuse IntuitionBase for pOS */
#else
#include <proto/intuition.h>
#endif

int has_fpu = 0;
int has_68010_or_up = 0;
int has_68020_or_up = 0;
int has_68030_or_up = 0;
int has_68040_or_up = 0;
int has_68060_or_up = 0;

#ifdef __MORPHOS__
#define GET_VA_ARRAY(x) __va_overflow(x)
#else
#define GET_VA_ARRAY(x) x
#endif

static int show_msg(char *title, const char *msg, char *ap, char *gadgetformat)
{
  struct IntuitionBase *IntuitionBase;
  int old_except;
  struct Task *me = SysBase->ThisTask;
  int ret = 0;

  /*
   * This function may be called with our signals enabled. So we have to make
   * sure that no signals are processed while in here.
   * Since this function may be called at random points in initialization,
   * it is not safe to call sigsetmask() here, so I have to resort to the
   * more drastic solution of temporarily turning off exceptions
   */
  old_except = SetExcept(0, ~0);

  if (title == NULL)
    title = "ixemul.library message";

#ifdef __pos__
  if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary("pintui.library", 0)))
#else
  if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0)))
#endif
    {
#ifdef __pos__
      struct pOS_EasyStruct panic = {
	sizeof(struct pOS_EasyStruct),
	0,
	ESYTYP_Request,
	title,
	(char *)msg,
	gadgetformat
      };

      ret = pOS_EasyRequestArgs(NULL, &panic, NULL, (const ULONG *)ap);
#else
      struct EasyStruct panic = {
	sizeof(struct EasyStruct),
	0,
	title,
	(char *)msg,
	gadgetformat
      };

      ret = EasyRequestArgs(NULL, &panic, NULL, ap);
#endif

      CloseLibrary ((struct Library *) IntuitionBase);
   }

#ifdef mc68000
  if (ixemulbase && (ix.ix_flags & ix_create_enforcer_hit) && has_68020_or_up)
    {
      asm ("movel #0,d0
	    move.l d0,0xdeaddead
	    nop
	    add.l #2,sp
	    move.l d0,0xdeaddead
	    nop
	    sub.l #2,sp" : /* no output */ : );
    }
#endif

  SetExcept(old_except, ~0);
  return ret;
}

void ix_panic(const char *msg, ...)
{
  va_list ap;
	
  va_start(ap, msg);
  show_msg(NULL, msg, GET_VA_ARRAY(ap), "Abort");
  va_end(ap);
}

void panic(const char *msg, ...)
{
  va_list ap;
	
  va_start(ap, msg);
  show_msg(NULL, msg, GET_VA_ARRAY(ap), "Abort");
  va_end(ap);
  abort();
}

void ix_warning(const char *msg, ...)
{
  va_list ap;
	
  va_start(ap, msg);
  if (!show_msg(NULL, msg, GET_VA_ARRAY(ap), "Continue|Abort"))
    exit(20);
  va_end(ap);
}

#ifdef NATIVE_MORPHOS
#define ix_vreq _varargs68k_ix_req
#endif

int ix_vreq(char *title, char *button1, char *button2, char *msg, char *ap)
{
  char *butfmt;
  int result;
	
  if (button1 == NULL)
    button1 = "Abort";
  if (button2 == NULL)
    button2 = "";
  butfmt = alloca(strlen(button1) + strlen(button2) + 2);
  strcpy(butfmt, button1);
  if (*button2)
  {
    strcat(butfmt, "|");
    strcat(butfmt, button2);
  }
  if (title == NULL)
  {
    title = alloca(40);
    if (!GetProgramName(title, 39))
      title = "Program message";
  }

  result = show_msg(title, msg, ap, butfmt);
  return result;
}

int ix_req(char *title, char *button1, char *button2, char *fmt, ...) __attribute__((varargs68k));

int ix_req(char *title, char *button1, char *button2, char *msg, ...)
{
  int ret;
  va_list ap;
  va_start(ap, msg);
  ret = ix_vreq(title, button1, button2, msg, GET_VA_ARRAY(ap));
  va_end(ap);
  return ret;
}

struct ixemul_base *ix_init_glue(struct ixemul_base *ixbase)
{
  /* First set SysBase, because it is used by the 'u' macro. The Enforcer
     manual recommends caching ExecBase because low-memory accesses are slower
     when using Enforcer, besides the extra penalty of being in CHIP memory.
     Also, lots of accesses to address 4 can hurt interrupt performance. */

#ifndef __pos__
  SysBase = *(struct ExecBase **)4;
#endif

  ixemulbase = ixbase;

#ifdef __pos__  
  has_fpu = pOS_ExecCheck(EXTSTTAG_FPU, TRUE, TAG_END);
  {
    long cpu_type;

    pOS_ExecCheck(EXTSTTAG_GetCPU, (long)&cpu_type, TAG_END);
    has_68060_or_up = cpu_type >= EXCPUTYP_68060;
    has_68040_or_up = cpu_type >= EXCPUTYP_68040;
    has_68030_or_up = cpu_type >= EXCPUTYP_68030;
    has_68020_or_up = cpu_type >= EXCPUTYP_68020;
    has_68010_or_up = cpu_type >= EXCPUTYP_68010;
  }
#else
  has_fpu = SysBase->AttnFlags & (AFF_68881 | AFF_68882);
  has_68060_or_up = (SysBase->AttnFlags & AFF_68060);
  has_68040_or_up = has_68060_or_up || (SysBase->AttnFlags & AFF_68040);
  has_68030_or_up = has_68040_or_up || (SysBase->AttnFlags & AFF_68030);
  has_68020_or_up = has_68030_or_up || (SysBase->AttnFlags & AFF_68020);
  has_68010_or_up = has_68020_or_up || (SysBase->AttnFlags & AFF_68010);
#endif

#if defined (mc68020) || defined (mc68030) || defined (mc68040) || defined (mc68060)
  if (!has_68020_or_up)
    {
      ix_panic("This ixemul version requires a 68020/30/40/60.");
      return NULL;
    }
#endif

#ifdef __HAVE_68881__
  if (!has_fpu)
    {
      if (has_68060_or_up)
	ix_panic("68060 math emulation code not found.");
      else if (has_68040_or_up)
	ix_panic("68040 math emulation code not found.");
      else
	ix_panic("This ixemul version requires an FPU.");
      return NULL;
    }
#endif

  return ix_init(ixbase);
}


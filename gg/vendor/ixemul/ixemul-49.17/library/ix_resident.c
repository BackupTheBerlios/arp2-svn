/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
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
 *  $Id: ix_resident.c,v 1.5 2000/10/04 17:53:52 emm Exp $
 *
 *  $Log: ix_resident.c,v $
 *  Revision 1.5  2000/10/04 17:53:52  emm
 *  * Fixed various problems with 68k ixemul programs
 *  * Completed support for 68k stack management
 *  * Improved configure/make
 *  * Fixed some includes bugs
 *  * Added support for ctors/dtors in crt0.o
 *  * Added the missing _err/_warn
 *  * Compiled the ixpipe: handler and some tools
 *
 *  Revision 1.4  2000/09/13 21:13:52  emm
 *  Works on 68k again
 *
 *  Revision 1.3  2000/09/05 21:00:02  emm
 *  Fixed some bugs. Deadlocks, memory trashing, ...
 *
 *  Revision 1.2  2000/06/20 22:17:22  emm
 *  First attempt at a native MorphOS ixemul
 *
 *  Revision 1.1.1.1  2000/05/07 19:38:07  emm
 *  Imported sources
 *
 *  Revision 1.1.1.1  2000/04/29 00:48:18  nobody
 *  Initial import
 *
 *  Revision 1.5  1994/06/19  19:30:31  rluebbert
 *  Optimization of subroutines
 *
 *  Revision 1.4  1994/06/19  15:13:14  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.2  1992/09/14  01:42:52  mwild
 *  if out of memory, exit. There's no reason to continue, the programs would
 *  just plain crash...
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <sys/exec.h>

#ifdef NATIVE_MORPHOS
#define A4_OFFSET (u.u_is_ppc ? 0x8000 : 0x7ffe)
#else
#define A4_OFFSET 0x7ffe
#endif

int
common_resident (int numpar, int a4, int databss_size, long *relocs)
{
  usetup;

  KPRINTF(("me = %lx, u = %lx, a4 = %lx, u_a4 = %lx, dsize = %lx, relocs = %lx, is_ppc = %ld\n", SysBase->ThisTask, u_ptr, a4, u.u_a4, databss_size, relocs, u.u_is_ppc));

  if ((numpar == 3 || numpar == 4) && databss_size)
    {
      void *old_sdata_ptr = u.u_sdata_ptr;
      int mem = (int)(u.u_sdata_ptr = kmalloc (databss_size + 15));
      if (! mem)
	{
	  ix_panic ("Out of memory.");
	  _exit (20);
	}
      else
	{
	  int origmem = a4 - A4_OFFSET;
	  mem = (mem + 15) & ~15;

	  KPRINTF(("a4 = %lx, origmem = %lx, mem = %lx\n", a4, origmem, mem));

	  memcpy((void *)mem, (void *)origmem, databss_size);

	  if (numpar == 4 && relocs[0] > 0)
	    {
	      int i, num_rel = relocs[0];

	      for (i = 0, relocs++; i < num_rel; i++, relocs++)
		  *(long *)(mem + *relocs) -= origmem - mem;
	    }

	  if (u.p_flag & SFREEA4)
	    kfree (old_sdata_ptr);
	  else
	    u.p_flag |= SFREEA4;

	  a4 = mem + A4_OFFSET;
	  KPRINTF(("relocated data: new a4 = %lx\n", a4));
	}
    }

  
  if (numpar >= 2 && numpar <= 4)
    {
      /* need output parameter, or the asm is optimized away */
      u.u_a4 = a4;
    }
  else
    ix_warning("Unsupported ix_resident call.");
  KPRINTF(("u_a4 = %lx\n", u.u_a4));
  return a4;
}

#ifdef NATIVE_MORPHOS

void
ix_resident (int numpar, int r13, int databss_size, long *relocs)
{
  usetup;
  u.u_is_ppc = 1;
  r13 = common_resident(numpar, r13, databss_size, relocs);
  asm volatile ("mr 13, %0" : "=r" (r13) : "0" (r13));
}

void
_trampoline_ix_resident (void)
{
  int *p = (int *)REG_A7;
  int numpar = p[1];
  int a4 = p[2];
  int databss_size = p[3];
  long *relocs = (long *)p[4];
  usetup;
  ULONG save[15];
  int k;

  /* Save the 68k regs, as they would be trashed by a call to EmulCallOS() */
  /* Don't use memcpy, as it uses CopyMem, and hence EmulCallOS(). */
  for (k = 0; k < 15; ++k)
    save[k] = ((ULONG *)MyEmulHandle)[k];

  u.u_is_ppc = 0;
  KPRINTF(("numpar = %ld, a4 = %lx, bsssize = %lx, relocs = %lx\n", numpar, a4, databss_size, relocs));

  a4 = common_resident(numpar, a4, databss_size, relocs);

  for (k = 0; k < 15; ++k)
    ((ULONG *)MyEmulHandle)[k] = save[k];

  REG_A4 = a4;
}

const struct EmulLibEntry _gate_ix_resident = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline_ix_resident
};


void ix_geta4 (void)
{
  usetup;
  if (u.u_a4) {
    asm volatile ("mr 13, %0" : "=r" (u.u_a4) : "0" (u.u_a4));
  }
}

void _trampoline_ix_geta4(void)
{
  usetup;
  KPRINTF(("%lx -> %lx\n", REG_A4, u.u_a4));
  if (u.u_a4)
      REG_A4 = u.u_a4;
}

const struct EmulLibEntry _gate_ix_geta4 = {
  TRAP_LIBRESTORE, 0, _trampoline_ix_geta4
};

#else

void
ix_resident (int numpar, int a4, int databss_size, long *relocs)
{
  a4 = common_resident(numpar, a4, databss_size, relocs);
  asm volatile ("movel %0, a4" : "=g" (a4) : "0" (a4));
}

void ix_geta4 (void)
{
  usetup;
  if (u.u_a4) {
    asm volatile ("movel %0, a4" : "=g" (u.u_a4) : "0" (u.u_a4));
  }
}

#endif

/* This function isn't used at the moment, but it is meant to be called
 * at the start of an executable. This function should check if the
 * executable can run on the CPU of this computer.
 */
void ix_check_cpu (int machtype)
{
#if notyet
  if (machtype == MID_SUN020)
    {
      if (!(SysBase->AttnFlags & AFF_68020))
	{
	  ix_panic ("This program requires at least a 68020 cpu.");
	  syscall (SYS_exit, 20);
	}
    }
#endif
}

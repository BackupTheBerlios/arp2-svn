/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
 *  Portions Copyright (C) 1995 Jeff Shepherd
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
 *  $Id: ix_startup.c,v 1.8 2003/12/13 02:56:54 zapek Exp $
 *
 *  $Log: ix_startup.c,v $
 *  Revision 1.8  2003/12/13 02:56:54  zapek
 *  network now defaults to amitcp
 *  removed a few warnings
 *
 *  Revision 1.7  2002/07/01 20:38:48  emm
 *  Got rid of __plock(). Beware of potential compatibility problems.
 *
 *  Revision 1.6  2002/06/14 17:23:30  laire
 *  removed __pos__. Needs a test(i couldn^t)
 *
 *  Revision 1.5  2000/09/18 21:28:19  emm
 *  Moved WB message handling in ix_open. Fixed a race condition in memory management.
 *
 *  Revision 1.4  2000/09/13 21:13:52  emm
 *  Works on 68k again
 *
 *  Revision 1.3  2000/07/23 19:17:35  emm
 *  Added ppc stack extension support. Improved glues. Fixed some bugs.
 *
 *  Revision 1.2  2000/06/20 22:17:24  emm
 *  First attempt at a native MorphOS ixemul
 *
 *  Revision 1.1.1.1  2000/05/07 19:38:09  emm
 *  Imported sources
 *
 *  Revision 1.1.1.1  2000/04/29 00:48:31  nobody
 *  Initial import
 *
 *  Revision 1.5  1994/06/19  15:13:22  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.3  1992/08/09  20:55:51  amiga
 *  import sysbase
 *
 *  Revision 1.2  1992/07/04  19:18:21  mwild
 *  remove SIGWINCH handler before returning
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include <sys/wait.h>
#include "kprintf.h"

int _main();

/*
 * Note: I kept the partition into startup and _main(), although in this
 *       case, both functions could be done in one function, since this is
 *       a library, and the user can't override _main anyway but globally...
 */

int
ix_startup (char *aline, int alen,
	    int expand, char *wb_default_window, u_int main, int *real_errno)
{
  struct Process *proc = (struct Process *)SysBase->ThisTask;
  struct user *u_ptr = getuser(proc);
  int exit_val;
  int fd;
  struct my_seg mySeg = { 0 };

  KPRINTF(("ix_startup(\"%s\", %ld, %ld\n", aline, alen, expand));
#ifdef NATIVE_MORPHOS
  u.u_is_ppc = !(main & 1);
#endif
  KPRINTF(("%lx: is_ppc = %ld\n", proc, u.u_is_ppc));

  /*
   * The following code to reset the fpu might not be necessary, BUT since
   * a CLI shell doesn't spawn a new process when executing a command - it
   * insteads calls the command like a subroutine - it depends on the Shell
   * whether the fpu is setup correctly. And I don't like to depend on any
   * thing ;-)
   */
  if (has_fpu)
    resetfpu();
  /* first deal with WB messages, since those HAVE to be answered properly,
   * even if we should fail later (memory, whatever..) */

  if (proc->pr_CLI)
    {
      struct CommandLineInterface *cli = (void *)BADDR(proc->pr_CLI);
      long segs;

      /* for usage by sys_exit() for example */
      KPRINTF (("CLI command line '%s'\n", aline));
      u.u_argline = aline;
      u.u_arglinelen = alen;
      u.u_segs = &mySeg;
      u.u_segs->name = NULL;
      segs = cli->cli_Module;
      u.u_segs->segment = segs;
      segs <<= 2;
      u.u_start_pc = segs + 4;
      u.u_end_pc = segs + *(long *)(segs - 4) - 8;
    }

  u.u_expand_cmd_line = expand;

  /* put some AmiTCP and AS225 in here since it can't go into ix_open
   * I need a pointer to the REAL errno
   */
  if (real_errno)
   {
      u.u_errno = real_errno;
      if (u.u_ixnetbase)
	netcall(NET_set_errno, real_errno, NULL);
   }

  KPRINTF (("&errno = %lx\n", real_errno));
/*{int* p=(int*)get_sp();
KPRINTF(("r1=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)get_68k_sp();
KPRINTF(("a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
}*/
  exit_val = _setjmp (u.u_jmp_buf);

  if (! exit_val)
    {
      /* from now on it's safe to allow signals */
      syscall (SYS_sigsetmask, 0);
      /* the first time thru call the program */
      KPRINTF (("calling __main()\n"));
      if (proc->pr_CLI)
	_main(aline, alen, main);
      else
	_main(u.u_wbmsg, wb_default_window, main);
      /* NORETURN */
    }
  /* in this case we came from a longjmp-call */
  exit_val = u.p_xstat;

  KPRINTF(("jumped back to ix_startup\n"));
/*{int* p=(int*)get_sp();
KPRINTF(("r1=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)get_68k_sp();
KPRINTF(("a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
}*/

  __ix_remove_sigwinch ();

  /* had to move the closing of files out of ix_close(), as close() may
     actually wait for the last packet to arrive, and inside ix_close() we're
     inside Forbid, and may thus never wait! */

  /* close all files */
  for (fd = 0; fd < NOFILE; fd++)
    if (u.u_ofile[fd]) syscall (SYS_close, fd);

  /* if at all possible, free memory before entering Forbid ! (Semaphore
     problems..) */
  all_free ();

  /*
   * If we were traced by a debugger who got us through a ptrace 'attach',
   * we need to unlink us from our parent and to give it a signal telling
   * it that we've died.
   */
  Forbid();
  if (u.p_pptr && u.p_pptr != (struct Process *) 1)
    send_death_msg(&u);
  Permit();

  KPRINTF(("ix_startup: exiting (%ld)\n", exit_val));
/*{int* p=(int*)get_sp();
KPRINTF(("r1=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)get_68k_sp();
KPRINTF(("a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
}*/

  return WEXITSTATUS(exit_val);
}

#ifdef NATIVE_MORPHOS

int
_trampoline_ix_startup (void)
{
  int *p = (int *)REG_A7;
  char *aline = (char *)p[1];
  int alen = p[2];
  int expand = p[3];
  char *wb_default_window = (char *)p[4];
  u_int mainfunc = p[5];
  int *real_errno = (int *)p[6];

  return ix_startup(aline, alen, expand, wb_default_window, mainfunc^1, real_errno);
}

struct EmulLibEntry _gate_ix_startup = {
  TRAP_LIB, 0, (void(*)())_trampoline_ix_startup
};

#endif


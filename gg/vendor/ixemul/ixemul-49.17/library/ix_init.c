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
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <exec/memory.h>
#include <dos/var.h>
#include <hardware/intbits.h>

#include <string.h>
#include <unistd.h>

#ifdef TRACK_ALLOCS
#undef AllocMem
#undef FreeMem
#define AllocMem(x,y) debug_AllocMem("init",x,y)
#define FreeMem(x,y) debug_FreeMem(x,y)
void *debug_AllocMem(const char *,int, int);
void debug_FreeMem(int,int);
#endif

/* not changed after the library is initialized */
struct ixemul_base              *ixemulbase = NULL;

struct ExecBase                 *SysBase = NULL;
struct DosLibrary               *DOSBase = NULL;

#ifndef __MORPHOS__
#ifndef __HAVE_68881__
struct MathIeeeSingBasBase      *MathIeeeSingBasBase = NULL;
struct MathIeeeDoubBasBase      *MathIeeeDoubBasBase = NULL;
struct MathIeeeDoubTransBase    *MathIeeeDoubTransBase = NULL;
#endif
#endif

struct Library                  *muBase = NULL;

static struct
{
  void          **base;
  char          *name;
  ULONG         minver;  /* minimal version          */
  BOOL          opt;     /* no fail if not available */
}
ix_libs[] =
{
  { (void **)&DOSBase, "dos.library", 37 },
#ifndef __MORPHOS__
#ifndef __HAVE_68881__
  { (void **)&MathIeeeSingBasBase, "mathieeesingbas.library" },
  { (void **)&MathIeeeDoubBasBase, "mathieeedoubbas.library" },
  { (void **)&MathIeeeDoubTransBase, "mathieeedoubtrans.library" },
#endif
#endif
  { NULL, NULL }
};


struct ixlist timer_wait_queue;
struct ixlist timer_ready_queue;
struct ixlist timer_task_list;
ULONG timer_resolution;

#ifndef NATIVE_MORPHOS
struct framemsg;
void handle_framemsg(struct framemsg *);
#endif
void safe_psignal(struct Task *, int);

static void ix_task_switcher(void)
{
  unsigned long sigs = 0;
  struct framemsg *msg;
  struct Task *me = SysBase->ThisTask;
  struct Task *caller = (void*) ix.ix_task_switcher_port;
  AllocSignal(29);
  AllocSignal(30);

  ix.ix_task_switcher      = me;
#ifndef NATIVE_MORPHOS
  ix.ix_task_switcher_port = CreateMsgPort();
#else
  ix.ix_task_switcher_port = NULL;
#endif
  Signal(caller, SIGBREAKF_CTRL_C);

  if (ix.ix_task_switcher_port)
    {
      // Signals:
      //
      // 1 << 29: Timer finished
      // 1 << 30: Global environment has changed

      /*
	This loop must not be interrupted by any other ixemul task
	outside of the Wait(), and this task must be scheduled
	right away when an ixemul task Signal()s it.
	However, Forbid()/Permit() is not necessary since the
	higher priority of this task has this effect.
      */
      /*Forbid();*/

      while (ix.ix_task_switcher == me)
      {
	KPRINTF(("Task switcher: waiting\n"));
	sigs = Wait((1 << ix.ix_task_switcher_port->mp_SigBit) |
		    (1 << 29) | (1 << 30) | SIGBREAKF_CTRL_C);

	KPRINTF(("Task switcher: got sigs %08lx\n", sigs));
	if (sigs & (1 << 30))
	  ix.ix_env_has_changed = 1;

#ifndef NATIVE_MORPHOS
	while (msg = (struct framemsg *)GetMsg(ix.ix_task_switcher_port))
	  {
	    KPRINTF(("Task switcher: handling framemsg\n"));
	    handle_framemsg(msg);
	  }
#endif
      }

      /*Permit();*/

      /*while (GetMsg(ix.ix_task_switcher_port));*/
#ifndef NATIVE_MORPHOS
      DeleteMsgPort(ix.ix_task_switcher_port);
#endif
    }
  else
    {
      while (ix.ix_task_switcher == me)
      {
	sigs = Wait((1 << 30) | SIGBREAKF_CTRL_C);

	if (sigs & (1 << 30))
	  ix.ix_env_has_changed = 1;
      }
    }

  Forbid();
  Signal(ix.ix_task_switcher, SIGBREAKF_CTRL_C);
  ix.ix_task_switcher = NULL;
}

int open_libraries(void)
{
  int i;

  for (i = 0; ix_libs[i].base; i++)
    if (!(*(ix_libs[i].base) = (void *)OpenLibrary(ix_libs[i].name, ix_libs[i].minver))
	&& !ix_libs[i].opt)
      {
	ix_panic("%s required!", ix_libs[i].name);
	return 0;
      }
  return 1;
}

void close_libraries(void)
{
  int i;

  for (i = 0; ix_libs[i].base; i++)
    if (*ix_libs[i].base)
      CloseLibrary(*ix_libs[i].base);
}

struct ixemul_base *ix_init (struct ixemul_base *ixbase)
{
  int i;
  char buf[256];
  extern char hostname[];  /* in getcrap.c */
  struct Task *me = SysBase->ThisTask;
  struct Task *task_switcher;

  ixbase->ix_debug_flag = 1;

  if (!open_libraries())
    {
      close_libraries();
      return 0;
    }

#if USE_DYNFILETAB
  NEWLIST((struct List *) &ixbase->ix_free_file_list);
  NEWLIST((struct List *) &ixbase->ix_used_file_list);
#else
  ixbase->ix_file_tab = (struct file *)AllocMem (NOFILE * sizeof(struct file), MEMF_PUBLIC | MEMF_CLEAR);
  ixbase->ix_fileNFILE = ixbase->ix_file_tab + NOFILE;
  ixbase->ix_lastf = ixbase->ix_file_tab;
#endif

  memset(&ixbase->ix_notify_request, 0, sizeof(ixbase->ix_notify_request));
  memset(&ixbase->ix_ptys, 0, sizeof(ixbase->ix_ptys));

  ixnewlist(&timer_wait_queue);
  ixnewlist(&timer_ready_queue);
  ixnewlist(&timer_task_list);
  ixnewlist(&ixbase->ix_detached_processes);

  timer_resolution = 1000000 / SysBase->VBlankFrequency;

  /* Read the GMT offset. This environment variable is 5 bytes long. The
     first 4 form a long that contains the offset in seconds and the fifth
     byte is ignored. */
  if (GetVar("IXGMTOFFSET", buf, 6, GVF_BINARY_VAR) == 5 && IoErr() == 5)
    ix_set_gmt_offset(*((long *)buf));
  else
    ix_set_gmt_offset(0);

  if (GetVar(IX_ENV_SETTINGS, buf, sizeof(struct ix_settings) + 1, GVF_BINARY_VAR) == sizeof(struct ix_settings))
    ix_set_settings((struct ix_settings *)buf);
  else
    ix_set_settings(ix_get_default_settings());

  /* Set the hostname if the environment variable HOSTNAME exists. */
  if (GetVar("HOSTNAME", buf, 64, 0) > 0)
    strcpy(hostname, buf);

  if (ixbase->ix_flags & ix_support_mufs)
    muBase = OpenLibrary("multiuser.library", 39);

  /* initialize the list structures for the allocator */
  init_buddy ();

  NewList(&ixbase->ix_framemsg_list);
  ixbase->ix_task_switcher_port = (void*) me;
  task_switcher = (struct Task *)ix_create_task("ixemul task switcher", 9, ix_task_switcher, 2048);
  KPRINTF(("created task %lx\n", task_switcher));
  if (task_switcher)
    {
      while (ixbase->ix_task_switcher_port == (void*) me)
	Wait(SIGBREAKF_CTRL_C);
    }

  KPRINTF(("Task switcher      = %lx\n", ixbase->ix_task_switcher));
  KPRINTF(("Task switcher port = %lx\n", ixbase->ix_task_switcher_port));

  ixbase->ix_notify_request.nr_stuff.nr_Signal.nr_Task = ixbase->ix_task_switcher;
  ixbase->ix_notify_request.nr_stuff.nr_Signal.nr_SignalNum = 30;
  ixbase->ix_notify_request.nr_Name = "ENV:";
  ixbase->ix_notify_request.nr_Flags = NRF_SEND_SIGNAL;

  /* initialize port number for AF_UNIX(localhost) sockets */
  ixbase->ix_next_free_port = 1024;

#ifdef NATIVE_MORPHOS
  if (ixbase->ix_task_switcher)
    {
      extern int ix_timer();

      ixbase->ix_itimerint.is_Node.ln_Type = NT_INTERRUPT;
      ixbase->ix_itimerint.is_Node.ln_Name = "ixemul timer interrupt";
      ixbase->ix_itimerint.is_Node.ln_Pri = 1;
      ixbase->ix_itimerint.is_Data = (APTR)0;
#ifdef NATIVE_MORPHOS
      {
	static struct EmulLibEntry gate = { TRAP_LIBSRNR, 0, (void(*)())ix_timer };
	ixbase->ix_itimerint.is_Code    = (APTR) &gate;
      }
#else
      ixbase->ix_itimerint.is_Code = (APTR)ix_timer;
#endif

      AddIntServer(INTB_VERTB, &ixbase->ix_itimerint);
    }
#endif

  if (
#if USE_DYNFILETAB
      1
#else
      ixbase->ix_file_tab
#endif
#ifndef NATIVE_MORPHOS
      && ixbase->ix_task_switcher_port
#endif
      )
    {
      for (i = 0; i < IX_NUM_SLEEP_QUEUES; i++)
	ixnewlist ((struct ixlist *)&ixbase->ix_sleep_queues[i]);

      ixbase->ix_global_environment = NULL;
      ixbase->ix_env_has_changed = 0;

      StartNotify(&ixbase->ix_notify_request);
      seminit();

      ixbase->ix_semaphore.lock.ss_Link.ln_Name = "Ixemul";
      ixbase->ix_semaphore.lock.ss_Link.ln_Pri = 0;
      ixbase->ix_semaphore.send_signal = safe_psignal;
      AddSemaphore(&ixbase->ix_semaphore.lock);

      return ixbase;
    }

  if (ixbase->ix_task_switcher)
    {
#ifdef NATIVE_MORPHOS
      RemIntServer(INTB_VERTB, &ixbase->ix_itimerint);
#endif

      Forbid();
      ixbase->ix_task_switcher = me;
      Signal(task_switcher, SIGBREAKF_CTRL_C);
      Permit();

      while (ixbase->ix_task_switcher == me)
	Wait(SIGBREAKF_CTRL_C);
    }

#if USE_DYNFILETAB
  {
    struct MinNode *node, *node2;
    ForeachNodeSafe(&ix.ix_free_file_list, node, node2)
    {
      kfree(node);
    }
    /* should be empty here! */
    ForeachNodeSafe(&ix.ix_used_file_list, node, node2)
    {
      kfree(node);
    }
    //NEWLIST((struct List *) &ixbase->ix_free_file_list);
    //NEWLIST((struct List *) &ixbase->ix_used_file_list);
  }
#else
  if (ixbase->ix_file_tab)
    FreeMem (ixbase->ix_file_tab, NOFILE * sizeof(struct file));
  else
    ix_panic ("out of memory");
#endif

  close_libraries();

  return 0;
}


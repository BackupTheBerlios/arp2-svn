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
 */

#define _KERNEL
#include "ixemul.h"

#include <hardware/intbits.h>

#ifdef TRACK_ALLOCS
#undef FreeMem
#define FreeMem(x,y) debug_FreeMem(x,y)
void debug_FreeMem(int,int);
void dump_memlist(void);
#endif

#ifndef NATIVE_MORPHOS
void delete_framemsgs();
#endif

void ix_expunge (struct ixemul_base *ixbase)
{
  extern long vector_install_count;     /* from trap.s */
  extern void *restore_vector;          /* from trap.s */
  struct Task *task;
  struct Task *me;

  RemSemaphore(&ixbase->ix_semaphore.lock);
  ObtainSemaphore(&ixbase->ix_semaphore.lock);

#ifdef NATIVE_MORPHOS
  RemIntServer(INTB_VERTB, &ixbase->ix_itimerint);
#endif
  EndNotify(&ixbase->ix_notify_request);

  task = ixbase->ix_task_switcher;
  me = SysBase->ThisTask;
  Forbid();
  ixbase->ix_task_switcher = me;
  Signal(task, SIGBREAKF_CTRL_C);
  Permit();

  while (ixbase->ix_task_switcher == me)
    Wait(SIGBREAKF_CTRL_C);

#ifndef NATIVE_MORPHOS
  delete_framemsgs();
#endif

  cleanup_buddy ();

  if (ixbase->ix_global_environment)
    {
      char **tmp = ixbase->ix_global_environment;

      while (*tmp)
	kfree(*tmp++);
      kfree(ixbase->ix_global_environment);
    }

  close_libraries();

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
  FreeMem (ixbase->ix_file_tab, NOFILE * sizeof (struct file));
#endif

#ifndef NATIVE_MORPHOS
  if (vector_install_count)
    {
      vector_install_count = 1;         /* force restoration original bus error vector */
      Supervisor(restore_vector);
    }
#endif

#ifdef TRACK_ALLOCS
  dump_memlist();
#endif
}

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
 *
 *  $Id: machdep.c,v 1.10 2001/07/02 16:24:16 emm Exp $
 *
 *  machdep.c,v
 *
 *  Revision ??? emm
 *  Replaced the launch() polling by task exceptions.
 *
 *  Revision 1.1.1.1  1994/04/04  04:30:40  amiga
 *  Initial CVS check in.
 *
 *  Revision 1.5  1993/11/05  21:59:18  mwild
 *  add code to deal with inet.library
 *
 *  Revision 1.4  1992/10/20  16:25:24  mwild
 *  no nasty 'c' polling in DEF-signalhandler...
 *
 *  Revision 1.3  1992/08/09  20:57:59  amiga
 *  add volatile to sysbase access, or the optimizer takes illegal shortcuts...
 *
 *  Revision 1.2  1992/07/04  19:20:20  mwild
 *  add yet another state in which not to force a context switch.
 *  Probably unnecessary paranoia...
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include <sys/wait.h>
#include "kprintf.h"

#include <string.h>
#include <exec/execbase.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/tasks.h>

void setrun (struct Task *t);
void sendsig(struct user *p, sig_t catcher, int sig, int mask, unsigned code, void *addr);
void resume_signal_check(void);

/* The signal handling is completely different than the one in previous versions.
 * It works in the following way:
 * A task exception handler is set during is_open() to catch all user
 * signals. If this exception handler decides that an Amiga signal should
 * result in a sighandler execution, it sends a message to the task switcher,
 * and exits.
 * Since the task switcher has a high priority, it is scheduled immediately.
 * The task switcher adds a new stack frame on the top of the old one,
 * with PC set to a trampoline function.
 * The task switcher then goes to sleep, and the task is rescheduled.
 * It executes the sighandler. The sighandler exits normally (i.e.
 * no longjmp() back), the trampoline tells the task switcher to
 * remove the added frame.
 */

#ifndef NATIVE_MORPHOS
/* messages that are sent by the exception handler to the task switcher task. */
struct framemsg {
  struct Message        fm_message;
  struct Task           *fm_task;       /* task to add a frame to */
  int                   fm_signum;      /* signo for handler */
  int                   fm_code;        /* additional info for handler */
  void                  *fm_addr;       /* yet another info for handler ;-)) */
  sig_t                 fm_handler;     /* handler addr for u_sigc */
  /*void                  *fm_sp;         /* sp to restore */
  int                   fm_state;       /* task state to restore */
  sigset_t              fm_waitsigs;    /* waiting signals to restore */
  struct sigcontext     fm_context;     /* context to restore */
};

/* Stack frames used to save the state of a task when it is rescheduled. */
struct frame_68000 {
    ULONG pc;
    UWORD sr;
    ULONG d[8];
    ULONG a[7];
};

struct frame_68060_idle_fpu {
    ULONG mode; /* 0x0 */
    ULONG pad0; /* 0x0 */
    ULONG pad1; /* 0x0 */
    ULONG pc;
    UWORD sr;
    ULONG d[8];
    ULONG a[7];
};

struct frame_68060_busy_fpu {
    ULONG  mode; /* 0xffffffff */
    ULONG  fpcr;
    ULONG  fpsr;
    ULONG  fpiar;
    ULONG  fp[3*8];
    ULONG  fpframe[3];
    ULONG  pc;
    UWORD  sr;
    ULONG  d[8];
    ULONG  a[7];
};

struct frame_idle_fpu {
    UWORD mode; /* 0x0 */
    UWORD pad;  /* 0x0 */
    ULONG pc;
    UWORD sr;
    ULONG d[8];
    ULONG a[7];
};

struct frame_busy_fpu {
    UWORD mode; /* 0xffff */
    ULONG fpcr;
    ULONG fpsr;
    ULONG fpiar;
    ULONG fp[3*8];
    UWORD fpframe[3];
    ULONG pc;
    UWORD sr;
    ULONG d[8];
    ULONG a[7];
};

/* Functions used for messages management. The error handling
 * currently sucks. */
struct framemsg *
create_framemsg()
{
  /* no ReplyPort, those messages won't be replied */
  return AllocMem(sizeof(struct framemsg), MEMF_PUBLIC | MEMF_CLEAR);
}

void
delete_framemsgs()
{
  struct Node *node;
  while ((node = ix.ix_framemsg_list.lh_Head)->ln_Succ)
    {
      Remove(node);
      FreeMem(node, sizeof(struct framemsg));
    }
}

struct framemsg *
get_framemsg()
{
  struct framemsg *fm;

  Forbid(); /* fixme: semaphores+signal mask would be better */

  if (IsListEmpty(&ix.ix_framemsg_list))
    {
      fm = create_framemsg();
    }
  else
    {
      fm = (struct framemsg *)ix.ix_framemsg_list.lh_Head;
      Remove(&fm->fm_message.mn_Node);
    }

  Permit();
  /* KPRINTF(("get_framemsg() = %lx\n", fm)); */
  return fm;
}


void
release_framemsg(struct framemsg *fm)
{
  /* KPRINTF(("release_framemsg(%lx)\n", fm)); */
  Forbid(); /* fixme: semaphores would be better */
  AddTail(&ix.ix_framemsg_list, &fm->fm_message.mn_Node);
  Permit();
}


/* this function is the one called by the new created frame. */
void volatile
sig_trampoline (struct framemsg *fm
#ifndef NATIVE_MORPHOS
				     asm("a0")
#endif
					       )
{
  struct Task      *me       = fm->fm_task;
  int               state    = fm->fm_state;
  sigset_t          waitsigs = fm->fm_waitsigs;
  int               signum   = fm->fm_signum;
  int               code     = fm->fm_code;
  void             *addr     = fm->fm_addr;
  sig_t             handler  = fm->fm_handler;
  struct sigcontext sc       = fm->fm_context;
  struct user      *u_ptr    = getuser(me);

  KPRINTF(("sig_trampoline(%lx, u = %lx) TDNestCnt=%ld IDNestCnt=%ld SR=%04lx\n", fm, u_ptr, SysBase->TDNestCnt, SysBase->IDNestCnt, REG_SR));

  release_framemsg(fm);

  KPRINTF(("handler = %lx\n", handler));

#ifdef NATIVE_MORPHOS

  if ((int)handler & 1)
    {
      GETEMULHANDLE
      struct EmulCaos caos;
      static const u_short glue[] = {
	  /* code for:
		movem.l d0/d1/a0/a1,-(sp)
		jsr     (a2)
		lea     16(sp),sp
		rts
	  */
	  0x48E7, 0xC0C0, 0x4E92, 0x4FEF, 0x0010, 0x4E75
      };
      caos.caos_Un.Function = &glue;
      caos.reg_d0 = signum;
      caos.reg_d1 = code;
      caos.reg_a0 = (ULONG)addr;
      caos.reg_a1 = (ULONG)&sc;
      caos.reg_a2 = (APTR)((int)handler & ~1);
      caos.reg_a4 = u.u_a4;
      MyEmulHandle->EmulCall68k(&caos);
    }
  else
    {
      KPRINTF(("is_ppc = %ld, r13 = %lx\n", u.u_is_ppc, u.u_r13));
      if (u.u_is_ppc && u.u_r13)
	  asm ("mr 13,%0" : : "r" (u.u_r13));
      ((void (*)())handler) (signum, code, addr, &sc);
    }

#else

  if (u.u_a4)
    asm ("movel %0,a4" : : "g" (u.u_a4));

  ((void (*)())handler) (signum, code, addr, &sc);

#endif


  if (handler == (sig_t)stopped_process_handler)
    resume_signal_check(); /* might lead to stack overflow ?? */

  u.u_onstack = sc.sc_onstack;
  u.p_sigmask = sc.sc_mask;

#ifndef NATIVE_MORPHOS
  /* tell the task switcher to restore the frame */
  while (!(fm = get_framemsg()))
    {
      /* Fixme: What else to do ? */
      /* Waiting is not safe, because we might break a Forbid,
	 but well... */
      KPRINTF(("Can't send signal\n"));
      Delay(100);
    }

  fm->fm_task          = me;
  fm->fm_handler       = NULL;
  fm->fm_context.sc_sp = sc.sc_sp;
  fm->fm_state         = state;
  fm->fm_waitsigs      = waitsigs;

  PutMsg(ix.ix_task_switcher_port, &fm->fm_message);
  Wait(0); /* In theory, this is never reached, but who knows... */
#else
  /**(int*)&me->tc_Flags = sc.sc_ap;*/
  me->tc_SigWait = waitsigs;
#endif
/*KPRINTF(("SR=%lx, type=%lx, flags=%lx\n",MyEmulHandle->SR,MyEmulHandle->Type,MyEmulHandle->Flags));
Disable();
{ struct Task * t;
for(t=(APTR)SysBase->TaskReady.lh_Head;t->tc_Node.ln_Succ;t=(APTR)t->tc_Node.ln_Succ)
    KPRINTF(("%lx: %s state=%ld pri=%ld sigwait=%lx sigreceived=%lx\n",t,t->tc_Node.ln_Name,t->tc_State,t->tc_Node.ln_Pri,t->tc_SigWait,t->tc_SigRecvd));
} Enable();*/

  KPRINTF(("end_sig_trampoline. TDNestCnt=%ld IDNestCnt=%ld\n", SysBase->TDNestCnt, SysBase->IDNestCnt));
}

/* This functions builds a new stack frame, according to the framemsg.
 * It is called by the task switcher. */
void
setup_frame(struct framemsg *fm)
{
  struct Task *task  = fm->fm_task;
  struct user *u_ptr = getuser(task);

  KPRINTF(("set_frame(task=%lx) TDNestCnt=%ld IDNestCnt=%ld\n", task, task->tc_TDNestCnt, task->tc_IDNestCnt));

  /* Save the current task state. */
  fm->fm_context.sc_sp      = (int)task->tc_SPReg;
  fm->fm_context.sc_ap      = *(u_int *)&task->tc_Flags;
  fm->fm_state              = task->tc_State;
  fm->fm_waitsigs           = task->tc_SigWait;

  /* Add the new frame. */
  /* We need to set pc, sr, fp control words, and a4 (for baserel support). */
  /* Also set fp data registers to 0, to avoid a possible exception if random */
  /* data are loaded into them. */
  /* In addition, a0 will be set to fm. */
  /* Other registers don't matter. */
#ifdef NATIVE_MORPHOS
    {
      struct TaskFrame68k frame;
      KPRINTF(("MorphOS PPC frame\n"));
      frame.PC = (void*)sig_trampoline;
      frame.SR = 0;
      frame.Xn[12] = u.u_a4;
      NewSetTaskAttrs(task, &frame, 0,
		      TASKINFOTYPE_PPC_NEWFRAME,
		      TASKTAG_PC, sig_trampoline,
		      TASKTAG_PPC_ARG1, fm,
		      TAG_END);
    }

#else
  if (has_fpu)
    {
      if (has_68060_or_up)
	{
	  if (*(int *)task->tc_SPReg == 0)
	    {
	      struct frame_68060_idle_fpu *q = task->tc_SPReg;
	      KPRINTF(("68060 idle fpu frame\n"));
	      --q;
	      q->mode = 0;
	      q->pad0 = 0;
	      q->pad1 = 0;
	      q->pc   = (int)sig_trampoline;
	      q->sr   = q[1].sr;
	      q->a[0] = (int)fm;
	      q->a[4] = u.u_a4;
	      task->tc_SPReg = q;
	    }
	  else
	    {
	      struct frame_68060_busy_fpu *q = task->tc_SPReg;
	      KPRINTF(("68060 busy fpu frame\n"));
	      --q;
	      q->mode       = 0xffffffff;
	      q->fpcr       = q[1].fpcr;
	      q->fpsr       = q[1].fpsr;
	      q->fpiar      = q[1].fpiar;
	      q->fpframe[0] = q[1].fpframe[0];
	      q->fpframe[1] = q[1].fpframe[1];
	      q->fpframe[2] = q[1].fpframe[2];
	      memset(q->fp, 0, sizeof(q->fp));
	      q->pc         = (int)sig_trampoline;
	      q->sr         = q[1].sr;
	      q->a[0]       = (int)fm;
	      q->a[4]       = u.u_a4;
	      task->tc_SPReg = q;
	    }
	}
      else
	{
	  if (*(short *)task->tc_SPReg == 0)
	    {
	      struct frame_idle_fpu *q = task->tc_SPReg;
	      KPRINTF(("idle fpu frame\n"));
	      --q;
	      q->mode = 0;
	      q->pad  = 0;
	      q->pc   = (int)sig_trampoline;
	      q->sr   = q[1].sr;
	      q->a[0] = (int)fm;
	      q->a[4] = u.u_a4;
	      task->tc_SPReg = q;
	    }
	  else
	    {
	      struct frame_busy_fpu *q = task->tc_SPReg;
	      KPRINTF(("busy fpu frame\n"));
	      --q;
	      q->mode       = 0xffff;
	      q->fpcr       = q[1].fpcr;
	      q->fpsr       = q[1].fpsr;
	      q->fpiar      = q[1].fpiar;
	      q->fpframe[0] = q[1].fpframe[0];
	      q->fpframe[1] = q[1].fpframe[1];
	      q->fpframe[2] = q[1].fpframe[2];
	      memset(q->fp, 0, sizeof(q->fp));
	      q->pc         = (int)sig_trampoline;
	      q->sr         = q[1].sr;
	      q->a[0]       = (int)fm;
	      q->a[4]       = u.u_a4;
	      task->tc_SPReg = q;
	    }
	}
    }
  else
    {
      struct frame_68000 *q = task->tc_SPReg;
      KPRINTF(("no-fpu frame\n"));
      --q;
      q->pc         = (int)sig_trampoline;
      q->sr         = q[1].sr;
      q->a[0]       = (int)fm;
      q->a[4]       = u.u_a4;
      task->tc_SPReg = q;
    }

  if (task->tc_State == TS_WAIT)
    {
      /* if the task is in a waiting state, it should be
	 waked up, otherwise the callback will not be
	 called immediately */

      KPRINTF(("waking up task %lx\n", task));
      Disable();
      Remove(&task->tc_Node);
      Enqueue(&SysBase->TaskReady, &task->tc_Node);
      task->tc_State = TS_RUN;
      Enable();
      KPRINTF(("done\n"));
    }
#endif
}

/* Remove the stack frame built above. This is just a matter
 * of resetting SP, and putting the task back to sleep if needed. */
/* This function is not used for MorphOS */
void
restore_frame(struct framemsg *fm)
{
  struct Task  *task = fm->fm_task;

  KPRINTF(("restore_frame(%lx)\n", task));
  task->tc_SPReg = (void*)fm->fm_context.sc_sp;

  if (fm->fm_state == TS_WAIT)
    {
      /* If task1 was waiting, it should be put back
	 to sleep, so as to avoid confusing Wait(). */

      KPRINTF(("putting task %lx back to sleep\n", task));
      Disable();
      Remove(&task->tc_Node);
      Enqueue(&SysBase->TaskWait, &task->tc_Node);
      task->tc_State = TS_WAIT;
      task->tc_SigWait = fm->fm_waitsigs;
      /* If waited signals occured during the exception, resend them */
      if ((task->tc_SigWait|task->tc_SigExcept) & task->tc_SigRecvd)
	Signal(task, task->tc_SigRecvd);
      Enable();
      KPRINTF(("done\n"));
    }
}

/* function called by the task switcher when it receives a framemsg */
void
handle_framemsg(struct framemsg *fm)
{
  if (fm->fm_handler)
    setup_frame(fm);
#ifndef NATIVE_MORPHOS
  else
    restore_frame(fm);
#endif
}
#endif

// Utility function for trap.s
struct Task *
get_current_task(void)
{
  return SysBase->ThisTask;
}


/*
 * Task exception handler. Called in user mode.
 */
#ifdef NATIVE_MORPHOS
sigset_t except_handler_func(void);

struct EmulLibEntry except_handler = {
  TRAP_LIB, 0, (void(*)()) except_handler_func
};

sigset_t
except_handler_func()
{
  sigset_t sigs = REG_D0;
#else
sigset_t
except_handler(sigset_t sigs asm("d0"))
{
#endif
  struct Task           *me             = SysBase->ThisTask;
  struct user           *u_ptr          = getuser(me);
  sigset_t              sigmsg          = sigmask (SIGMSG);
  sigset_t              sigint          = sigmask (SIGINT);
  sigset_t              newsigs;
  sigset_t		oldsigwait;
  sigset_t		oldexceptsigs;
  int                   i;
  int                   onstack;
  int                   sigmask;

#ifdef NATIVE_MORPHOS
  Disable();
  me->tc_SigRecvd |= sigs;
  Enable();
#else
  me->tc_SigRecvd |= sigs;
#endif

  KPRINTF(("Exception handler entered (%08lx, %08lx, %ld, %ld) SR = %04lx\n",
	   sigs, me->tc_SigExcept, SysBase->TDNestCnt, SysBase->IDNestCnt, REG_SR));
  if (u.u_mask_state) /* do not handle signals while the stop-handler is running */
    goto out2;

  /* if we're inside ix_sleep, no signal processing is done to break the
     Wait there as soon as possible. Signals resume on return of ix_sleep */
  /* Likewise if the process is stopped for debugging (SSTOP).  */
  if (u.p_stat == SSLEEP || u.p_stat == SSTOP)
    goto out2;

  oldsigwait = me->tc_SigWait;
  oldexceptsigs = u.u_exceptsigs;
  u.u_exceptsigs = sigs;
  onstack = u.u_onstack;
  sigmask = u.p_sigmask;

  /* special processing for Wait()ing in Commodore inet.library. They
     do reasonable interrupt checking, but only on SIGBREAKF_CTRL_C. So
     whenever we have a signal to deliver, send ^C.. */
  if (u.p_stat == SWAIT)
    {

      KPRINTF(("Sending CTRL-C\n"));

      /* Forbid should be enough, but exec doesn't reschedule when
	 the handler exits, so if an interrupt activates a high priority
	 task while we are in Forbid state, when the exception handler
	 exits, the readylist is in an inconsistent state */
#ifdef NATIVE_MORPHOS
      Forbid();
#else
      Disable();
#endif

      //onstack = u.u_onstack;
      //sigmask = u.p_sigmask;
      if (CURSIG(&u))
	Signal (me, SIGBREAKF_CTRL_C);
      goto out;
    }

  /* smells kludgy I know...... */
  /* Emm: we can't enter an exception handler in Forbid/Disable state
  if (me->tc_TDNestCnt >= 0 || me->tc_IDNestCnt >= 0)
    goto out2;
  */

  /*
   * first check AmigaOS signals. If SIGMSG is set to SIG_IGN or SIG_DFL, 
   * we do our default mapping of SIGBREAKF_CTRL_C into SIGINT.
   */
  newsigs = sigs & ~u.u_lastrcvsig;
  u.u_lastrcvsig |= sigs;

  KPRINTF(("ignore = %08lx, catch = %08lx, newsigs = %08lx\n",
	   u.p_sigignore, u.p_sigcatch, newsigs));

#ifdef NATIVE_MORPHOS
  Forbid();
#else
  Disable();
#endif

  if (u.u_ixnetbase)
    netcall(NET__siglaunch, newsigs);

  if (((u.p_sigignore & sigmsg) || !(u.p_sigcatch & sigmsg)) 
      && (newsigs & SIGBREAKF_CTRL_C))
    {
      /* in that case send us a SIGINT, if it's not ignored */
      if (!(u.p_sigignore & sigint))
	{
	  struct Process *proc = (struct Process *)(u.u_session ? u.u_session->pgrp : (int)me);
	  _psignalgrp(proc, SIGINT);
	}
	
      /* in this mode we fully handle and use SIGBREAKF_CTRL_C, so remove it
       * from the Exec signal mask */
       
#ifdef NATIVE_MORPHOS
      Disable();
#endif
      me->tc_SigRecvd &= ~SIGBREAKF_CTRL_C;
      u.u_lastrcvsig &= ~SIGBREAKF_CTRL_C;
#ifdef NATIVE_MORPHOS
      Enable();
#endif
    }
  else if (newsigs && (u.p_sigcatch & sigmsg))
    {
      /* if possible, deliver the signal directly to get a code argument */
      if (!(u.p_flag & STRC) && !(u.p_sigmask & sigmsg))
	{
	  u.u_ru.ru_nsignals++;

	  KPRINTF(("reenable $%08lx\n", sigs));

	  sendsig(&u, u.u_signal[SIGMSG], SIGMSG, u.p_sigmask, newsigs, 0);
	  u.p_sigmask |= u.u_sigmask[SIGMSG] | sigmsg;

	  //setrun (me);
	}
      else
	_psignal (me, SIGMSG);
    }

  if ((i = CURSIG(&u)))
    {
      psig(&u, i);
    }

out:
  u.u_exceptsigs = oldexceptsigs;
  me->tc_SigWait = oldsigwait;
  u.u_onstack = onstack;
  u.p_sigmask = sigmask;
out2:
/*KPRINTF(("SR=%lx, type=%lx, flags=%lx\n",MyEmulHandle->SR,MyEmulHandle->Type,MyEmulHandle->Flags));
Disable();
{ struct Task *t;
for(t=(APTR)SysBase->TaskReady.lh_Head;t->tc_Node.ln_Succ;t=(APTR)t->tc_Node.ln_Succ)
    KPRINTF(("%lx: %s state=%ld pri=%ld sigwait=%lx sigreceived=%lx\n",t,t->tc_Node.ln_Name,t->tc_State,t->tc_Node.ln_Pri,t->tc_SigWait,t->tc_SigRecvd));
} Enable();*/
  KPRINTF(("Exception handler exiting (%ld, %ld).\n", SysBase->TDNestCnt, SysBase->IDNestCnt));
  return sigs;
}

/*
 * Send an interrupt to process.
 * Called from psig() which is called from sig_launch, thus we are in
 * an exception handler, in user mode, and in Forbid() state.
 */
void
sendsig (struct user *p, sig_t handler, int sig, int mask, unsigned code, void *addr)
{
  struct Task           *me = SysBase->ThisTask;
  struct framemsg       *fm;
  int                   oonstack;
  struct sigcontext     sc;
  struct user           *u_ptr = getuser(me);
  sigset_t		exceptsigs;

  KPRINTF(("sendsig(%lx, %08lx, %ld, %lx)\n", sig, mask, code, addr));
  KPRINTF(("TDNestCnt = %ld, IDNestCnt = %ld, pri=%ld\n", SysBase->TDNestCnt, SysBase->IDNestCnt, me->tc_Node.ln_Pri));

  oonstack = p->u_onstack;

  if (!p->u_onstack && (p->u_sigonstack & sigmask(sig)))
    p->u_onstack = 1;

#ifdef NATIVE_MORPHOS 

  sc.sc_onstack = oonstack;
  sc.sc_mask = mask;

  exceptsigs = u.u_exceptsigs;
  u.u_exceptsigs = 0;
  me->tc_SigExcept |= exceptsigs;

  Permit();

  if ((int)handler & 1)
    {
      struct EmulCaos caos;
      static const u_short glue[] = {
	  /* code for:
		movem.l d0/d1/a0/a1,-(sp)
		jsr     (a2)
		lea     16(sp),sp
		rts
	  */
	  0x48E7, 0xC0C0, 0x4E92, 0x4FEF, 0x0010, 0x4E75
      };
      caos.caos_Un.Function = &glue;
      caos.reg_d0 = sig;
      caos.reg_d1 = code;
      caos.reg_a0 = (ULONG)addr;
      caos.reg_a1 = (ULONG)&sc;
      caos.reg_a2 = (APTR)((int)handler & ~1);
      caos.reg_a4 = u.u_a4;
      MyEmulHandle->EmulCall68k(&caos);
    }
  else
    {
      int old_r13;
      KPRINTF(("is_ppc = %ld, r13 = %lx\n", u.u_is_ppc, u.u_r13));
      if (u.u_is_ppc && u.u_r13)
	  asm volatile ("mr %0,13; mr 13,%1" : "=&r" (old_r13) : "r" (u.u_r13) : "r13");
      ((void (*)())handler) (sig, code, addr, &sc);
      if (u.u_is_ppc && u.u_r13)
	  asm volatile ("mr 13,%0" : : "r" (old_r13) : "r13");
    }

  Forbid();

  me->tc_SigExcept &= ~exceptsigs;
  u.u_exceptsigs = exceptsigs;

  u.u_onstack = sc.sc_onstack;
  u.p_sigmask = sc.sc_mask;

#else
  /* make room for dummy stack frame (used by GDB) */
  /******usp -= 8;
  dummy_frame = (int *)usp;*/

  while (!(fm = get_framemsg()))
    {
      /* Fixme: What to do ? */
      /* Ignoring the signal is not safe... */
      /* Waiting is not safe either, because we are in an exception
	 handler, and also because we might break a Forbid, but well... */
      KPRINTF(("Can't send signal\n"));
      Delay(100);
    }

  /* fill out the frame */
  fm->fm_task               = me;
  fm->fm_signum             = sig;
  fm->fm_code               = code;
  fm->fm_addr               = addr;
  fm->fm_handler            = handler;
  fm->fm_context.sc_onstack = oonstack;
  fm->fm_context.sc_mask    = mask;
  KPRINTF(("Sending message %lx\n", fm));
  PutMsg(ix.ix_task_switcher_port, &fm->fm_message);

/*KPRINTF(("SR=%lx, type=%lx, flags=%lx\n",MyEmulHandle->SR,MyEmulHandle->Type,MyEmulHandle->Flags));
Disable();
{ struct Task *t;
for(t=(APTR)SysBase->TaskReady.lh_Head;t->tc_Node.ln_Succ;t=(APTR)t->tc_Node.ln_Succ)
    KPRINTF(("%lx: %s state=%ld pri=%ld sigwait=%lx sigreceived=%lx\n",t,t->tc_Node.ln_Name,t->tc_State,t->tc_Node.ln_Pri,t->tc_SigWait,t->tc_SigRecvd));
} Enable();*/
#endif

  KPRINTF(("end sendsig()\n"));
}


/*
 * called as the default action of a signal that terminates the process
 */
void
sig_exit (unsigned int code)
{
  /* the whole purpose of this code inside is to
   * prettyprint and identify the job that just terminates
   * This stuff should be handled by a shell, but since there's (yet) no
   * shell that knows how to interpret a signal-exit code I have to do it
   * here myself...
   */

  extern char *sys_siglist[NSIG];
  extern void exit2(int);
  struct Process *me = (struct Process *)SysBase->ThisTask;
  struct user *u_ptr = getuser(me);
  char err_buf[255];
#ifndef __pos__
  struct CommandLineInterface *cli;
#endif
  char process_name[255];
  int is_fg;

  KPRINTF(("sig_exit(%ld)\n", code));

  /* make sure we're not interrupted in this last step.. */
  u.p_flag &= ~STRC;    /* disable tracing */
  syscall (SYS_sigsetmask, ~0);
#ifdef mc68000
  if ((ix.ix_flags & ix_create_enforcer_hit) && has_68020_or_up)
    {
      /* This piece of assembly will skip all the saved registers, signal
	 contexts, etc. on the stack and set the stack pointer accordingly.
	 The number 700 has been determined by experimenting. After setting
	 the SP it will put the exit code into the address 0xDEADDEAD. This
	 creates an Enforcer hit, and Enforcer will show a stack dump starting
	 with the new SP. If we didn't add 700 bytes to the SP, then you would
	 have to configure Enforcer to use a stacktrace of more than 24 lines
	 before you would get to the relevant parts of the trace. After that we
	 add a nop and restore the SP. The nop was needed because due to the
	 pipelining of a 68040 the SP was already updated before Enforcer could
	 read the value of the SP. More nops may be needed for the 68060 CPU. */

      Forbid();
      asm ("movel %0,d0
	    addw  #700,sp
	    movel d0,0xdeaddead
	    nop
	    addqw #2,sp
	    movel d0,0xdeaddead
	    nop
	    addaw #-702,sp" : /* no output */ : "a" (code));
      Permit();
    }
#endif

  /* output differs depending on
   *  o  whether we're a CLI or a WB process (stderr or requester)
   *  o  whether this is a foreground or background process
   *  o  whether this is SIGINT or not
   */

#ifdef __pos__
  if (1)
    {
      if (!pOS_GetProgramName(process_name, sizeof(process_name)))
	process_name[0] = 0;

      is_fg = 1;
    }
#else
  if ((cli = BTOCPTR (me->pr_CLI)))
    {
      if (!GetProgramName(process_name, sizeof(process_name)))
	process_name[0] = 0;

      is_fg = cli->cli_Interactive && !cli->cli_Background;
    }
#endif
  else
    {
      process_name[0] = 0;
      if (me->pr_Task.tc_Node.ln_Name)
	strncpy (process_name, me->pr_Task.tc_Node.ln_Name, sizeof (process_name) - 1);
	
      /* no WB process is ever considered fg */
      is_fg = 0;
    }
  /* if is_fg and SIGINT, simulate tty-driver and display ^C */
  if (!is_fg || (code != SIGINT))
    {
      strcpy (err_buf, (code < NSIG) ? sys_siglist[code] : "Unknown signal");

      /* if is_fg, don't display the job */
      if (! is_fg)
	{
	  strcat (err_buf, " - ");
	  strcat (err_buf, process_name);
	  /* if we're a CLI we have an argument line saved, that we can print
	   * as well */
#ifndef __pos__
	  if (cli)
#endif
	    {
	      int line_len;
	      char *cp;
	      
	      /* we can display upto column 77, this should be safe on all normal
	       * amiga CLI windows */
	      line_len = 77 - strlen (err_buf) - 1;
	      if (line_len > u.u_arglinelen)
		line_len = u.u_arglinelen;

	      if (line_len > 0 && u.u_argline)
		{
		  strcat (err_buf, " ");
		  strncat (err_buf, u.u_argline, line_len);
		}

	      /* now get rid of possible terminating line feeds/cr's */
	      for (cp = err_buf; *cp && *cp != '\n' && *cp != '\r'; cp++) ;
	      *cp = 0;
	    }
	}

#ifdef __pos__
      if (1)
#else
      if (cli)
#endif
	{
	  /* uniformly append ONE line feed */
	  strcat (err_buf, "\n");
	  syscall (SYS_write, 2, err_buf, strlen (err_buf));
	}
      else
	ix_panic (err_buf);
    }
  else
    syscall (SYS_write, 2, "^C\n", 3);

  KPRINTF(("Calling exit2()\n"));
  exit2(W_EXITCODE(0, code));
  /* not reached */
}

/*
 * This is used to awaken a possibly sleeping sigsuspend()
 * and to force a context switch, if we send a signal to ourselves
 */
/* We no longer force a context switch, since tc_Launch is not used
 * anymore. Instead, we call psig() directly.
 */

void
setrun (struct Task *t)
{
  struct user *p  = getuser(t);
  struct Task *me = SysBase->ThisTask;
  u_int sr;

  KPRINTF (("setrun(%lx, %ld)\n", t, p->p_stat));
  KPRINTF (("p_stat = %ld, TDNestCnt = %ld, IDNestCnt = %ld\n", p->p_stat, SysBase->TDNestCnt, SysBase->IDNestCnt));
  /* NOTE: the context switch is done to make sure sig_launch() is called as
   *       soon as possible in the respective task. It's not nice if you can
   *       return from a kill() to yourself, before the signal handler had a
   *       chance to react accordingly to the signal..
   */
#ifdef NATIVE_MORPHOS
  sr = REG_SR;
#else
    {
      asm volatile ("
	movel a5,a0
	lea   L_get_sr,a5
	movel 4:w,a6
	jsr   a6@(-0x1e)
	movel a1,%0
	bra   L_skip
    L_get_sr:
	movew sp@,a1        | get sr register from the calling function
	rte
    L_skip:
	movel a0,a5
	    " : "=g" (sr) : : "a0", "a1", "a6");
    }
#endif

  /* Don't force context switch if:
     o  running in Supervisor mode
     o  we setrun() some other process
     o  running under either Forbid() or Disable() */
  if ((sr & 0x2000)
      || me != t
      || p->p_stat == SSLEEP
      || p->p_stat == SWAIT
      || p->p_stat == SSTOP
#ifdef NATIVE_MORPHOS
      || p->u_exceptsigs == 0)
#else
      || SysBase->TDNestCnt >= 0
      || SysBase->IDNestCnt >= 0)
#endif
    {
      extern int select();

      /* make testing of p_stat and reaction atomic */
      Forbid();

      if (p->p_stat == SWAIT)
	{
	  KPRINTF (("sending CTRL-C\n", t, p->p_stat));
	  Signal (t, SIGBREAKF_CTRL_C);
	}
      else if (p->p_stat == SSTOP)
	{
	  p->p_stat = SRUN;
	  KPRINTF (("sending zombie_sig\n", t, p->p_stat));
	  Signal (t, 1 << p->p_zombie_sig);
	}
      else if (p->p_wchan == (caddr_t) p)
	{
	  KPRINTF (("setrun $%lx\n", p));
	  ix_wakeup ((u_int)p);
	}
      else if (p->p_stat == SSLEEP || p->p_wchan == (caddr_t) select)
	{
	  KPRINTF (("sending sleep_sig\n", t, p->p_stat));
	  Signal (t, 1<<p->u_sleep_sig);
	}
      else if (t != me || p->u_exceptsigs == 0)
	{
	  KPRINTF (("signaling $%lx\n", t));
	  Signal (t, SIGBREAKF_CTRL_F);
	}

      Permit();
    }
  /*else
    {
      int i;
      if ((i = CURSIG(p)))
	{
	  psig(p, i);
	}
    }*/
  KPRINTF (("end setrun()\n"));
}

#ifndef NOTRAP
/*
 * Mapping from vector numbers into signals
 */
const static int hwtraptable[256] = {
  SIGILL, /* Reset initial stack pointer */
  SIGILL, /* Reset initial program counter */
  SIGBUS, /* Bus Error */
  SIGBUS, /* Address Error */
  SIGILL, /* Illegal Instruction */
  SIGFPE, /* Zero Divide */
  SIGFPE, /* CHK, CHK2 Instruction */
  SIGFPE, /* cpTRAPcc, TRAPcc, TRAPV Instruction */
  SIGILL, /* Privilege Violation */
  SIGTRAP,/* Trace */
  SIGEMT, /* Line 1010 Emulator */
  SIGEMT, /* Line 1111 Emulator */
  SIGILL,
  SIGILL, /* Coprocessor Protocol Violation */
  SIGILL, /* Format Error */
  SIGILL, /* Uninitialized Interrupt */
  SIGILL, /* 16 */
  SIGILL, /* 17 */
  SIGILL, /* 18 */
  SIGILL, /* 19 */              /* unimplemented, reserved */
  SIGILL, /* 20 */
  SIGILL, /* 21 */
  SIGILL, /* 22 */
  SIGILL, /* 23 */
  SIGILL, /* spurious Interrupt */
  SIGILL, /* Level 1 Interrupt Autovector */
  SIGILL, /* Level 2 Interrupt Autovector */
  SIGILL, /* Level 3 Interrupt Autovector */
  SIGILL, /* Level 4 Interrupt Autovector */
  SIGILL, /* Level 5 Interrupt Autovector */
  SIGILL, /* Level 6 Interrupt Autovector */
  SIGILL, /* Level 7 Interrupt Autovector */
  SIGTRAP, /* Trap #0 (not available on Unix) */
  SIGTRAP, /* Trap #1 */
  SIGILL, /* Trap #2 */
  SIGILL, /* Trap #3 */
  SIGILL, /* Trap #4 */
  SIGILL, /* Trap #5 */
  SIGILL, /* Trap #6 */
  SIGILL, /* Trap #7 */
  SIGILL, /* Trap #8 */
  SIGILL, /* Trap #9 */
  SIGILL, /* Trap #10 */
  SIGILL, /* Trap #11 */
  SIGILL, /* Trap #12 */
  SIGILL, /* Trap #13 */
  SIGILL, /* Trap #14 */
  SIGILL, /* Trap #15 (not available on Unix) */
  SIGFPE, /* FPCP Branch or Set on Unordererd Condition */
  SIGFPE, /* FPCP Inexact Result */
  SIGFPE, /* FPCP Divide by Zero */
  SIGFPE, /* FPCP Underflow */
  SIGFPE, /* FPCP Operand Error */
  SIGFPE, /* FPCP Overflow */
  SIGFPE, /* FPCP Signaling NAN */
  SIGILL,
  SIGBUS, /* MMU Configuration Error */
  SIGILL, /* MMU Illegal Operation (only 68851) */
  SIGILL, /* MMU Privilege Violation (only 68851) */
  /* rest undefined or free user-settable.. */
};

/*
 * handle traps handed over from the lowlevel trap handlers
 */
void
trap (void)
{
  u_int                 format;
  void                  *addr;
  struct reg            *regs;
  struct fpreg          *fpregs;
  struct Task           *me = SysBase->ThisTask;
  /* precalculate struct user, so we don't have to go thru SysBase all the time */
  struct user           *p = getuser(me);
  int                   sig;
  u_int                 usp, orig_usp;
  extern long           vector_old_pc;
  extern long           vector_nop;

  KPRINTF (("trap()\n"));

#if 0
  usp = orig_usp = get_usp () ;/*+ 8;      /* skip argument parameters */
  format = ((u_int *)usp)[0];
  addr = (void *)((u_int *)usp)[1];
  regs = (struct reg *)((u_int *)usp)[2];
  fpregs = (struct fpreg *)((u_int *)usp)[3];

  if (regs->r_pc == (void *)(&vector_nop) + 2)
    {
      regs->r_pc = (void *)vector_old_pc;
      vector_old_pc = 0;
    }
  p->u_regs = regs;
  p->u_fpregs = fpregs;
  /* format contains the vector * 4, in the lower 12 bits */
  sig = *(int *)((u_char *)hwtraptable + (format & 0x0fff));
  
  if (sig == SIGTRAP)
    regs->r_sr &= ~0x8000;      /* turn off the trace flag */

  trapsignal (me, sig, format, addr);

  if ((sig = CURSIG(p)))
    psig (p, sig);
#endif
  psig(p, SIGKILL);

  KPRINTF (("end trap()\n"));
}
#endif

#ifndef NATIVE_MORPHOS

void resume_signal_check(void)
{
  int sig;
  usetup;
  if ((sig = issig(&u)))  /* always go through issig if we restart the process */
    psig (&u, sig);
}

#endif


#define _GNU_SOURCE // for ucontext register access
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <ucontext.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "options.h"
//#include "events.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
//#include "cpu_prefetch.h"
//#include "autoconf.h"
//#include "ersatz.h"
//#include "debug.h"
//#include "gui.h"
//#include "savestate.h"
//#include "blitter.h"
//#include "ar.h"
#include "blomcall.h"

int in_blomcall = 0;

static int                      blomcall_enable = 0;
static timer_t                  blomcall_timer;
static struct blomcall_stack*   blomcall_stack;
static struct blomcall_context* blomcall_context;

static const struct itimerspec blomcall_timer_off = {
  { 0, 0 }, { 0, 0 }
};

static const struct itimerspec blomcall_timer_once = {
  { 0, 0 }, { 0, 1000000 } // 1 ms
};


void ix86_switch();
void ix86_switch_end();
void _ix86_switch() {
__asm__("				\n\
ix86_switch:				\n\
	leal	%c0(%%ebx),%%eax	\n\
	pushl	%%eax			\n\
	call	getcontext		\n\
	addl	$4,%%esp		\n\
	testl	%%eax,%%eax		\n\
	jnz	1f			\n\
	leal	%c1(%%ebx),%%eax	\n\
	pushl	$2			\n\
	pushl	%%eax			\n\
	call	siglongjmp		\n\
1:					\n\
	pushl	16(%%ebx)		\n\
	pushl	20(%%ebx)		\n\
	movl	0(%%ebx),%%eax		\n\
	movl	4(%%ebx),%%edx		\n\
	movl	8(%%ebx),%%ecx		\n\
	decl	24(%%ebx)		\n\
	movl	12(%%ebx),%%ebx		\n\
	popf				\n\
	ret				\n\
ix86_switch_end:			\n\
	"
	:
	: "i" (&((struct blomcall_context*) 0)->ix86context),
	  "i" (&((struct blomcall_context*) 0)->emuljmp)
	: "memory");
}


static void blomcall_sighandler(int x, struct sigcontext sc) {
  static int cnt;

  if ((cnt++ % 1000) == 0) {
    printf("%d\n", cnt);
  }

//  printf("timer\n");
//  stack = (struct blomcall_stack*) get_real_address (m68k_getpc ());

  // Make sure the blomcall stack is OK
  if (blomcall_stack == NULL) {
    printf ("No blomcall stack!\n");
    return;
  }

  if (blomcall_context == NULL) {
    printf ("No blomcall context!\n");
    return;
  }

  if (blomcall_context->disable ||
      (sc.eip >= ix86_switch && sc.eip < ix86_switch_end)) {
    printf("timer disabled!\n");
    return;
  }
  
//  printf("timer\n");
//  printf(".");
  blomcall_context->disable = 1;
  blomcall_context->eax     = sc.eax;
  blomcall_context->edx     = sc.edx;
  blomcall_context->ecx     = sc.ecx;
  blomcall_context->ebx     = sc.ebx;
  blomcall_context->eip     = sc.eip;
  blomcall_context->eflags  = sc.eflags;
//  printf("eax: %x, edx: %x, ecx: %x\n", sc.eax, sc.edx, sc.ecx);
//  printf("ebx: %x, eip: %x, esp: %x\n", sc.ebx, sc.eip, sc.esp);

  // Go back to emulation
 sc.ebx = blomcall_context;
 sc.eip = ix86_switch;
}


static void call_blomcall(struct blomcall_stack* stack) REGPARAM;
static void REGPARAM2 call_blomcall(struct blomcall_stack* stack)
{
  printf("call_blomcall (stack: op: %04x pc: %08x a7: %08x ...\n",
	 stack->op_resume, stack->pc, stack->a7);

  while(1);// {printf("*");}
//    ((void (*)(void)) addr)();
  siglongjmp(blomcall_context->emuljmp, 1);
}


int blomcall_init () {
  struct sigaction sa;

  if (timer_create (CLOCK_REALTIME, NULL, &blomcall_timer) == -1) {
    perror("timer_create");
    return 0;
  }

  sa.sa_handler = blomcall_sighandler;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGALRM, &sa, NULL) == -1 ) {
    perror("sigaction");
    return 0;
  }
  
  blomcall_enable = 1;
  return 1;
}


unsigned long blomcall_ops (uae_u32 opcode) {
  if (!blomcall_enable) {
    return op_illg (opcode);
  }

  if (opcode != OP_BRESUME) {
    uae_u32 newpc  = get_long (m68k_getpc() + 2);
    uae_u8* addr   = get_real_address(newpc);
    uae_u32 old_a7 = m68k_areg (regs, 7);
    uaecptr a7;

    printf("blomcall to %08lx (%08lx: %02x %02x\n",
	   newpc, addr, addr[0], addr[1]);

    printf("a7 before blomcall: %x (real address %p)\n",
	   m68k_areg (regs, 7), get_real_address (m68k_areg (regs, 7)));

    a7 = m68k_areg (regs, 7) -= sizeof (struct blomcall_stack);

    blomcall_stack = get_real_address (a7);

    blomcall_context = malloc (sizeof (struct blomcall_context));

    blomcall_stack->op_resume = OP_BRESUME;
    blomcall_stack->zero      = 0;
    blomcall_stack->pc        = m68k_getpc ();
    blomcall_stack->a7        = old_a7;
    blomcall_stack->context   = blomcall_context;

    m68k_setpc (a7);
  }
  else {
    blomcall_stack   = get_real_address (m68k_getpc());
    blomcall_context = blomcall_stack->context;
  }

  switch (sigsetjmp (blomcall_context->emuljmp, 0)) {
    case 0: {
      if (timer_settime(blomcall_timer, 0, &blomcall_timer_once, NULL) == -1) {
	perror("timer_settime");
	abort();
      }

      in_blomcall = 1;
      blomcall_context->disable = 0;

      if (opcode != OP_BRESUME) {
	__asm__ __volatile__ ("\
		movl  %0,%%eax  \n\
		movl  %0,%%esp	\n\
		pushl %1	\n\
		ret\n"
			      :
			      : "r" (blomcall_stack), "r" (call_blomcall)
			      : "memory", "eax");
      }
      else {
	blomcall_context->ix86context.uc_mcontext.gregs[REG_EAX] = 1;
	setcontext(&blomcall_context->ix86context);
      }

      // Not reached
      abort();
    }

    case 1: {
      printf("blomcall returned\n");

      in_blomcall = 0;
	
      m68k_setpc (blomcall_stack->pc);
      m68k_areg (regs, 7) = blomcall_stack->a7;
      printf("restored pc: %08x, a7: %08x\n",
	     blomcall_stack->pc, blomcall_stack->a7);
      m68k_incpc(6);
      free(blomcall_context);
      break;
    }

    case 2: {
      uae_u32 stack_usage;

      printf("back from blomcall because of alarm\n");

      // Check stack usage and adjust a7
      stack_usage = (sizeof (struct blomcall_stack) +
		     (intptr_t) blomcall_stack -
		     blomcall_context->ix86context.uc_mcontext.gregs[REG_ESP]);
      m68k_areg (regs, 7) = stack->a7 - stack_usage;

      printf("adjusted stack: %d bytes used\n", stack_usage);
      break;
  }

  timer_settime(blomcall_timer, 0, &blomcall_timer_off, NULL);
  blomcall_stack   = NULL;
  blomcall_context = NULL;
}

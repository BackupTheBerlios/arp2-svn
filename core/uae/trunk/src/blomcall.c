
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

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

volatile int blomcall_cycles;
volatile int blomcall_counter;
volatile int blomcall_code;

#define limited_cycles(x) ((x) < 1000 ? (x) : 1000)

static int                      blomcall_enable = 0;
static timer_t                  blomcall_timer;
static sigset_t                 blomcall_usr1sigset;
static struct blomcall_context* blomcall_context;

static const struct itimerspec blomcall_timer_cont = {
  { 0, 1e9/1100 }, { 0, 1e9/1100 } // 1100 Hz
};

static void blomcall_timer_handler(int x, struct sigcontext sc) {
  ++blomcall_counter;

  if (blomcall_context == NULL) {
    abort();
    return;
  }

  blomcall_context->sc = sc;
  blomcall_context->fp = *sc.fpstate;

//  printf("saving state to context %p at %p\n", blomcall_context, sc.eip);
  
  // Quick sigprocmask()
  sc.oldmask |= sigmask(SIGUSR1);
  siglongjmp(blomcall_context->emuljmp, 2);
}

static void blomcall_resume_handler(int x, struct sigcontext sc) {
  if (blomcall_context == NULL) {
    abort();
    return;
  }

  struct _fpstate* fpp = sc.fpstate; 

  sc = blomcall_context->sc;
  *fpp = blomcall_context->fp;
  sc.fpstate = fpp;

//  printf("resuming from sighandler: %p\n", sc.eip);
}

int blomcall_init (void) {
  struct sigaction sa;
  struct sigevent ev;

  sigemptyset(&blomcall_usr1sigset);
  sigaddset(&blomcall_usr1sigset, SIGUSR1);

  ev.sigev_notify = SIGEV_SIGNAL;
  ev.sigev_signo  = SIGUSR1;

  if (timer_create (CLOCK_REALTIME, &ev, &blomcall_timer) == -1) {
    perror("timer_create");
    return 0;
  }

  sa.sa_handler = (__sighandler_t) blomcall_timer_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGUSR1, &sa, NULL) == -1 ) {
    perror("sigaction(SIGUSR1)");
    return 0;
  }

  sa.sa_handler = (__sighandler_t) blomcall_resume_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGUSR2, &sa, NULL) == -1 ) {
    perror("sigaction(SIGUSR2)");
    return 0;
  }
  
  if (sigprocmask(SIG_BLOCK, &blomcall_usr1sigset, NULL) == -1) {
    perror("sigprocmask");
    return 0;
  }

  if (timer_settime(blomcall_timer, 0, &blomcall_timer_cont, NULL) == -1) {
    perror("timer_settime");
    return 0;
  }
  
  blomcall_enable = 1;
  return 1;
}


static void blomcall_exit(uae_u64 rc) REGPARAM;
static void REGPARAM2 blomcall_exit(uae_u64 rc)
{
  sigprocmask(SIG_BLOCK, &blomcall_usr1sigset, NULL);
  m68k_dreg (regs, 0) = (uae_u32) rc;          // eax -> d0
  m68k_dreg (regs, 1) = (uae_u32) (rc >> 32);  // edx -> d1
  siglongjmp(blomcall_context->emuljmp, 1);
}


static void blomcall_exitnr(void)
{
  sigprocmask(SIG_BLOCK, &blomcall_usr1sigset, NULL);
  siglongjmp(blomcall_context->emuljmp, 1);
}


unsigned long blomcall_ops (uae_u32 opcode) {
  static unsigned long remaining_cycles = 0;
  unsigned long cycles;
  uae_u64 start_time;
  uae_u64 call_time;
  uae_u8* ix86addr;

  cycles = limited_cycles(remaining_cycles);

  if (cycles != 0) {
    remaining_cycles -= cycles;
    return cycles;
  }

  // Sanity check
  if (!blomcall_enable) {
    return op_illg (opcode);
  }

  start_time = read_processor_time ();

  if (opcode != OP_BRESUME) {
    uae_u32 newpc  = get_long (m68k_getpc() + 2);

    ix86addr = get_real_address (newpc);

    printf("blomcall to %08lx (%08lx: %02x %02x\n",
	   newpc, ix86addr, ix86addr[0], ix86addr[1]);

    printf("a7 before blomcall: %x (real address %p)\n",
	   m68k_areg (regs, 7), get_real_address (m68k_areg (regs, 7)));

    // TODO: Cache and reuse used contexts
    blomcall_context             = malloc (sizeof (struct blomcall_context));
    blomcall_context->pc         = m68k_getpc ();
    blomcall_context->a7         = m68k_areg (regs, 7);
    blomcall_context->real_a7    = (uae_u32*)
      get_real_address (blomcall_context->a7);
    blomcall_context->rts_pc     = blomcall_context->real_a7[0];
    blomcall_context->real_a7[0] = (opcode == OP_BCALL ?
				    (uae_u32) blomcall_exit :
				    (uae_u32) blomcall_exitnr);
  }
  else {
    struct blomcall_stack* blomcall_stack;

    blomcall_stack   = (struct blomcall_stack*) get_real_address (m68k_getpc());
    blomcall_context = blomcall_stack->context;

    m68k_areg (regs, 7) += sizeof (struct blomcall_stack);    
    
//    printf("blomresume: stack=%08x context=%p\n", blomcall_stack, blomcall_context);
  }

  switch (sigsetjmp (blomcall_context->emuljmp, 0)) {
    case 0: {
      if (opcode != OP_BRESUME) {
//	printf("jmp\n");
//	sigprocmask(SIG_UNBLOCK, &blomcall_usr1sigset, NULL);
	__asm__ __volatile__ ("		\n\
		movl  %0,%%esp		\n\
		pushl %4		\n\
		pushl $0		\n\
		pushl %1		\n\
		pushl %2		\n\
		call  sigprocmask	\n\
		addl  $12,%%esp		\n\
		leal  %c3,%%eax		\n\
		ret"
			      : :
			      "m" (blomcall_context->real_a7),
			      "i" (&blomcall_usr1sigset),
			      "i" (SIG_UNBLOCK),
			      "i" (&regs.regs),
			      "r" (ix86addr) :
			      "memory", "eax");
      }
      else {
//	printf("kill\n");
	kill(getpid(), SIGUSR2);
      }

      // Never reached
      abort();
    }

    case 1: {
      printf("blomcall returned\n");

      m68k_setpc (blomcall_context->pc);
      m68k_areg (regs, 7) = blomcall_context->a7;
      blomcall_context->real_a7[0] = blomcall_context->rts_pc;
      
      printf("restored pc: %08x, a7: %08x\n",
	     blomcall_context->pc, blomcall_context->a7);
      m68k_incpc(6);
      free(blomcall_context);
      break;
    }

    case 2: {
      struct blomcall_stack* blomcall_stack;

//      printf("back from blomcall because of alarm\n");

      // Check stack usage, add blomcall_stack and adjust a7
      uae_u32 stack_usage = ((intptr_t) blomcall_context->real_a7 -
			     (intptr_t) blomcall_context->sc.esp +
			     sizeof (struct blomcall_stack));
      m68k_areg (regs, 7) = blomcall_context->a7 - stack_usage;

      blomcall_stack = (struct blomcall_stack*)
	((intptr_t) blomcall_context->real_a7 - stack_usage);

      blomcall_stack->op_resume = OP_BRESUME;
      blomcall_stack->zero      = 0;
      blomcall_stack->context   = blomcall_context;
      m68k_setpc (m68k_areg (regs, 7));

//      printf("stack usage: %d bytes. a7=%08x (%p). pc is %08x\n", stack_usage,
//	     m68k_areg (regs, 7), blomcall_stack, m68k_getpc());
      break;
    }
  }

  blomcall_context = NULL;
//  printf("going back\n");

  call_time = read_processor_time () - start_time;
//  printf("call_time: %lld, syncbase: %ld\n", call_time, syncbase);

  blomcall_cycles += (unsigned long) ((double) call_time / syncbase * 7.09e6 * 0.5);
//  printf("%10.3g; ",  (double) call_time * 16 / 2e9 * 7.14e6 * 256);

  remaining_cycles = (unsigned long) ((double) call_time / syncbase * 7.09e6 * 0.5);
  remaining_cycles *= 3;
  cycles = limited_cycles(remaining_cycles);
  remaining_cycles -= cycles;
  return cycles;
}

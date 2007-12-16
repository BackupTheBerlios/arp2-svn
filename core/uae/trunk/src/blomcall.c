 /*
  * UAE - The Un*x Amiga Emulator
  *
  * BJMP (etc.) support code
  *
  * Copyright 2005 Martin Blom
  *
  * Features:
  *
  * 1) "Multitasking" native code (emulation keeps running).
  * 2) Native code uses m68k stack (you definately want SA_ONSTACK!).
  * 3) m68k-to-native and native-to-m68k calls use no extra stack, so it's
  *    possible to pass parameters on the stack on i386.
  * 5) Arguments can be in d0-d7/a0-a6/fp0-fp7 or stack via sp.
  * 4) Fast short-circuit of native-to-native calls via BJMP.
  */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <inttypes.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "machdep/m68kops.h"
#include "blomcall.h"
#include "uae_endian.h"

#if !defined(NATMEM_OFFSET) || NATMEM_OFFSET != 0
# error NATMEM_OFFSET must be 0!
#endif

/*** Macros ******************************************************************/

// Execute half a raster line at the time
#define limited_cycles(x) ((x) < 113 ? (x) : 113)

#if defined (__i386__)
//# define get_jmpbuf_sp(jb) ((uintptr_t) ((jb)[0]->__jmpbuf[JB_SP]))
# define get_sigctx_sp(bc) ((uintptr_t) ((bc)->uc.uc_mcontext.esp))
#elif defined (__x86_64__)
//# define get_jmpbuf_sp(jb) ((uintptr_t) ((jb)[0]->__jmpbuf[JB_RSP]))
# define get_sigctx_sp(bc) ((uintptr_t) ((bc)->uc.uc_mcontext.rsp))
#else
# error Unsupported architecture!
#endif


/*** Globals *****************************************************************/

static int                      blomcall_enable = 0;
static timer_t                  blomcall_timer;
static sigset_t                 blomcall_usr1sigset;
static sigset_t                 blomcall_usr2sigset;
static __thread struct blomcall_context* blomcall_ctx     = NULL;
static __thread struct blomcall_segment* blomcall_segment = NULL;

#define SIGHANDLER_STACK_SIZE 65536 /* Remember that the UAE debugger
				       runs on this stack too! */
static void* sighandler_stack;

static const struct itimerspec blomcall_timer_cont = {
  { 0, 1e9/2000 }, { 0, 1e9/2000 } // 2 kHz (or less, depending on your OS and tick rate)
};


/*** Direct 1:1 access memory ************************************************/

static uae_u32 directmem_lget (uaecptr) REGPARAM;
static uae_u32 directmem_wget (uaecptr) REGPARAM;
static uae_u32 directmem_bget (uaecptr) REGPARAM;
static void directmem_lput (uaecptr, uae_u32) REGPARAM;
static void directmem_wput (uaecptr, uae_u32) REGPARAM;
static void directmem_bput (uaecptr, uae_u32) REGPARAM;
static int directmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *directmem_xlate (uaecptr addr) REGPARAM;

uae_u32 REGPARAM2 directmem_lget (uaecptr addr)
{
    return do_get_mem_long ((uae_u32*) (uintptr_t) addr);
}

uae_u32 REGPARAM2 directmem_wget (uaecptr addr)
{
    return do_get_mem_word ((uae_u16*) (uintptr_t) addr);
}

uae_u32 REGPARAM2 directmem_bget (uaecptr addr)
{
    return *((uae_u8*) (uintptr_t) addr);
}

void REGPARAM2 directmem_lput (uaecptr addr, uae_u32 l)
{
    do_put_mem_long ((uae_u32*) (uintptr_t) addr, l);
}

void REGPARAM2 directmem_wput (uaecptr addr, uae_u32 w)
{
    do_put_mem_word ((uae_u16*) (uintptr_t) addr, w);
}

void REGPARAM2 directmem_bput (uaecptr addr, uae_u32 b)
{
    *((uae_u8*) (uintptr_t) addr) = (uae_u8) b;
}

int REGPARAM2 directmem_check (uaecptr addr, uae_u32 size)
{
    return 1;
}

uae_u8 REGPARAM2 *directmem_xlate (uaecptr addr)
{
    return (uae_u8*) (uintptr_t) addr;
}

addrbank directmem_bank = {
    directmem_lget, directmem_wget, directmem_bget,
    directmem_lput, directmem_wput, directmem_bput,
    directmem_xlate, directmem_check, NULL
};


/*** FS/GS segment handling **************************************************/

#if defined (__linux__) && defined (__i386__)

#include <asm/ldt.h>
#include <linux/unistd.h>

#define LDT_SEL(idx) ((idx) << 3 | 1 << 2 | 3)

int modify_ldt(int func, void *ptr, unsigned long bytecount);

static int set_segment(void* ptr, int len) {
  const int sel = 0;
  struct modify_ldt_ldt_s ldt = {
    sel,                        // entry_number
    (long) ptr,                 // base_addr
    len,                        // limit
    1,                          // seg_32bit
    MODIFY_LDT_CONTENTS_DATA,   // contents
    0,                          // read_exec_only
    0,                          // limit_in_pages
    0,                          // seg_not_present
    1,                          // useable
  };

  if (modify_ldt(1, &ldt, sizeof (ldt)) == -1) {
    perror("modify_ldt(1)");
    return 0;
  }

  __asm volatile ("movw %w0,%%fs" : : "r" (LDT_SEL(sel)));

  return 1;
}

static inline int read_segment(int offset) {
  int rc;

  __asm volatile ("movl %%fs:(%1), %0" : "=r" (rc) : "r" (offset));

  return rc;
}

#elif defined (__linux__) && defined (__x86_64__)

#include <asm/prctl.h>
#include <sys/prctl.h>

int arch_prctl(int code, unsigned long addr);

static int set_segment(void* ptr, int len) {
  (void) len;
  
  if (arch_prctl(ARCH_SET_GS, (unsigned long) ptr) == -1) {
    perror("arch_prctl");
    return 0;
  }

  return 1;
}

static inline int read_segment(int offset) {
  int rc;

  __asm volatile ("movl %%gs:%c1, %0" : "=r" (rc) : "i" (offset));

  return rc;
}

#else
# error Unsupported architecture!
#endif


static int install_segment(void) {
  size_t const length = 20;
  
  if (blomcall_segment == NULL) {
    blomcall_segment = (struct blomcall_segment*) valloc(length);
  }

  if (blomcall_segment != NULL) {
    if (set_segment(blomcall_segment, length)) {
      assert ((((uae_u64) blomcall_callfunc68k) & 0xffffffff00000000ULL) == 0);
      assert ((((uae_u64) blomcall_calllib68k) & 0xffffffff00000000ULL) == 0);

      blomcall_segment->length   = length;
      blomcall_segment->execbase = 0;
      blomcall_segment->userdata = 0;
      blomcall_segment->callfunc = (uae_u32) (uintptr_t) blomcall_callfunc68k;
      blomcall_segment->calllib  = (uae_u32) (uintptr_t) blomcall_calllib68k;
    }
    else {
      free (blomcall_segment);
      blomcall_segment = NULL;
    }
  }

  return blomcall_segment != NULL;
}


/*** Code to return back from native code ************************************/

uae_u32 __blomcall_callfunc68k(uae_u32 args[16], fptype fp[8], uae_u32 addr) REGPARAM;
uae_u32 REGPARAM2 __blomcall_callfunc68k(uae_u32 args[16], fptype fp[8], uae_u32 addr) {

  assert (blomcall_ctx != NULL);

  // Adjust sp not to include native return address (blomcall_ops() will push
  // correct m68k address)
  args[8+7] += sizeof (void*);
  
  // Save UAE's register array, load a new one
  memcpy(blomcall_ctx->saved_regs, blomcall_ctx->regs->regs, 16*4);
  memcpy(blomcall_ctx->regs->regs, args, 16*4);

  if (fp != NULL) {
    memcpy(blomcall_ctx->saved_fpregs, blomcall_ctx->regs->fp, 8*4);
    memcpy(blomcall_ctx->regs->fp, fp, 8*4);
  }

  // Save errno
  blomcall_ctx->saved_errno = errno;

  // Set emulation address
  m68k_setpc(blomcall_ctx->regs, addr);
  fill_prefetch_slow(blomcall_ctx->regs);

  // Since glibc now mangles the JB_*SP member, we can't use that to
  // fetch the stack pointer anymore, so we have to use inline
  // assembly and wish for luck. :-(
#if defined (__i386__)
  __asm volatile ("movl %%esp, %0" : : "m" (blomcall_ctx->stack_pointer));
#elif defined (__x86_64__)
  __asm volatile ("movq %%rsp, %0" : : "m" (blomcall_ctx->stack_pointer));
#else
# error Unsupported architecture!
#endif
  blomcall_ctx->stack_pointer -= 32; // Assume the sigsetjmp() won't
				     // use more than 32 bytes or
				     // stack. I deserve to die.

  // Save return address in m68kjmp ...
  if (sigsetjmp (blomcall_ctx->m68kjmp, 0) == 0) {
    // ... and head back to emulation via the emuljmp
    pthread_sigmask(SIG_BLOCK, &blomcall_usr1sigset, NULL);
    siglongjmp(blomcall_ctx->emuljmp, 3);
  }

  assert (blomcall_ctx != NULL);

  // Now we're back from emulation again
  pthread_sigmask(SIG_UNBLOCK, &blomcall_usr1sigset, NULL);

  // Restore errno
  errno = blomcall_ctx->saved_errno;

  // Transfer UAE's register array back to caller, then restore it
  memcpy(args, blomcall_ctx->regs->regs, 16*4);
  memcpy(blomcall_ctx->regs->regs, blomcall_ctx->saved_regs, 16*4);

  if (fp != NULL) {
    memcpy(fp, blomcall_ctx->regs->fp, 8*4);
    memcpy(blomcall_ctx->regs->fp, blomcall_ctx->saved_fpregs, 8*4);
  }
  
  // Return d0 for convenience
  return args[0];
}


// These are the entry points for native-to-m68k calls, implemented
// in assembly for benchmperformance reasons. They:
//
// 1) Store the native stack pointer in a7. This is the only part
//    that really requires assembly code.
// 2) Check if the opcode at <addr> is BJMP/BJMPNR/... 
//    * If so, fetch the address after the opcode, byteswap and
//      jump directly to the native function, with <args>
//      (%eax/%rdi) and stack intact.
//    * If not, jump directly to blomcall_callfunc68k(). <args>
//      (%eax/%rdi), <addr> (%edx/%rsi) and the stack are still valid.
//
// Additionally, blomcall_calllib68k() simply adds a6 to <offset> to
// form an address first.
//
// For 64-bit machines, the stack must obviously be in the 32-bit
// address space, since a7 is just 32 bit long, but BJMP & co takes
// care of that.
//

// uae_u32 blomcall_calllib68k(uae_u32 args[16], fptype fp[8], uae_u32 offset) REGPARAM;
// uae_u32 blomcall_callfunc68k(uae_u32 args[16], fptype fp[8], uae_u32 addr) REGPARAM;

#if defined (__i386__)

__asm ("						\n\
	.p2align 2,,3					\n\
	.globl	blomcall_calllib68k 			\n\
	.type	blomcall_calllib68k,@function 		\n\
blomcall_calllib68k: 					\n\
	add	(8+6)*4(%eax),%ecx 			\n\
	mov	%esp,(8+7)*4(%eax) 			\n\
	cmpb	$0xfe,(%ecx)				\n\
	jne	__blomcall_callfunc68k		 	\n\
	mov	2(%ecx),%ecx				\n\
	bswap	%ecx					\n\
	jmp	*%ecx					\n\
							\n\
	.p2align 2,,3					\n\
	.globl	blomcall_callfunc68k 			\n\
	.type	blomcall_callfunc68k,@function		\n\
blomcall_callfunc68k: 					\n\
	mov	%esp,(8+7)*4(%eax) 			\n\
	cmpb	$0xfe,(%ecx)				\n\
	jne	__blomcall_callfunc68k		 	\n\
	mov	2(%ecx),%ecx				\n\
	bswap	%ecx					\n\
	jmp	*%ecx					\n\
");

# define blomcall_exit   __blomcall_exit 

#elif defined (__x86_64__)

__asm ("						\n\
	.p2align 4,,15					\n\
	.globl	blomcall_calllib68k 			\n\
	.type	blomcall_calllib68k,@function 		\n\
blomcall_calllib68k: 					\n\
	add	(8+6)*4(%rdi),%edx 			\n\
	mov	%esp,(8+7)*4(%rdi) 			\n\
	cmpb	$0xfe,(%rdx)				\n\
	jne	__blomcall_callfunc68k		 	\n\
	mov	2(%rdx),%edx				\n\
	bswap	%edx					\n\
	jmp	*%rdx					\n\
							\n\
	.p2align 4,,15					\n\
	.globl	blomcall_callfunc68k 			\n\
	.type	blomcall_callfunc68k,@function	 	\n\
blomcall_callfunc68k: 					\n\
	mov	%esp,(8+7)*4(%rdi) 			\n\
	cmpb	$0xfe,(%rdx)				\n\
	jne	__blomcall_callfunc68k		 	\n\
	mov	2(%rdx),%edx				\n\
	bswap	%edx					\n\
	jmp	*%rdx					\n\
							\n\
	.p2align 4,,15					\n\
	.type	blomcall_exit,@function		 	\n\
blomcall_exit: 						\n\
	mov	%rax,%rdi				\n\
	jmp	__blomcall_exit				\n\
");

void blomcall_exit(uae_u32 rc) REGPARAM;

#else
# error Unsupported architecture!
#endif

static void blomcall_exitnr(void)
{
  assert (blomcall_ctx != NULL);

  pthread_sigmask(SIG_BLOCK, &blomcall_usr1sigset, NULL);
  siglongjmp(blomcall_ctx->emuljmp, 1);
}

static void __blomcall_exit(uae_u32 rc) REGPARAM;
static void REGPARAM2 __attribute__((used)) __blomcall_exit(uae_u32 rc)
{
  assert (blomcall_ctx != NULL);

  m68k_dreg (blomcall_ctx->regs, 0) = rc;          // eax -> d0

  // tst.l d0 -- so interrupt handlers work without extra glue code
  CLEAR_CZNV (&blomcall_ctx->regs->ccrflags);
  SET_ZFLG (&blomcall_ctx->regs->ccrflags, rc == 0);
  SET_NFLG (&blomcall_ctx->regs->ccrflags, (uae_s32) rc < 0);

  return blomcall_exitnr();
}


/*** Signal handlers *********************************************************/

static void blomcall_timer_handler(int x, siginfo_t* si, void* extra) {
  struct kernel_ucontext* uc = extra;

  assert (blomcall_ctx != NULL);

  blomcall_ctx->uc = *uc;
  blomcall_ctx->fp = *uc->uc_mcontext.fpstate;

  // Save errno
  blomcall_ctx->saved_errno = errno;

  // SIGUSR1 already blocked, since we're in a signal handler
  // pthread_sigmask(SIG_BLOCK, &blomcall_usr1sigset, NULL);
  siglongjmp(blomcall_ctx->emuljmp, 2);
}

static void blomcall_resume_handler(int x, siginfo_t* si, void* extra) {
  struct kernel_ucontext* uc = extra;

  assert (blomcall_ctx != NULL);

  // fpstate is a pointer in Linux!
  struct _fpstate* fpp = uc->uc_mcontext.fpstate;

  *uc = blomcall_ctx->uc;
  *fpp = blomcall_ctx->fp;
  uc->uc_mcontext.fpstate = fpp;

  // Restore errno
  errno = blomcall_ctx->saved_errno;
}


/*** Initialization code *****************************************************/

int blomcall_init (void) {
  stack_t ss;
  struct sigaction sa;
  struct sigevent ev;

  // Create and install a new stack for all signal handlers. This is
  // important since we're executing on AmigaOS' stack, which can be
  // really tiny.
  ss.ss_sp = sighandler_stack = malloc(SIGHANDLER_STACK_SIZE);
  ss.ss_flags = 0;
  ss.ss_size = SIGHANDLER_STACK_SIZE;

  if (sigaltstack(&ss, NULL) == -1) {
    perror("sigaltstack");
  }
  
  // Install signal handler for SIGUSR1 (timer) and SIGUSR2
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;

  sa.sa_sigaction = blomcall_timer_handler;
  if (sigaction(SIGUSR1, &sa, NULL) == -1 ) {
    perror("sigaction(SIGUSR1)");
    return 0;
  }

  sa.sa_sigaction = blomcall_resume_handler;
  if (sigaction(SIGUSR2, &sa, NULL) == -1 ) {
    perror("sigaction(SIGUSR2)");
    return 0;
  }
  
  // Block SIGUSR1 signals (from the timer) 
  sigemptyset(&blomcall_usr1sigset);
  sigaddset(&blomcall_usr1sigset, SIGUSR1);
  
  if (pthread_sigmask(SIG_BLOCK, &blomcall_usr1sigset, NULL) == -1) {
    perror("pthread_sigmask");
    return 0;
  }

  // Block SIGUSR2 signals
  sigemptyset(&blomcall_usr2sigset);
  sigaddset(&blomcall_usr2sigset, SIGUSR2);

  if (pthread_sigmask(SIG_BLOCK, &blomcall_usr2sigset, NULL) == -1) {
    perror("pthread_sigmask");
    return 0;
  }
  
  // Create a timer that generates SIGUSR1 signals
  ev.sigev_notify = SIGEV_SIGNAL;
  ev.sigev_signo  = SIGUSR1;

  if (timer_create(CLOCK_REALTIME, &ev, &blomcall_timer) == -1) {
    perror("timer_create");
    return 0;
  }

  // Start it (but signals are still blocked)
  if (timer_settime(blomcall_timer, 0, &blomcall_timer_cont, NULL) == -1) {
    perror("timer_settime");
    return 0;
  }

  return 1;
}


int blomcall_reset(void) {
  // Unblock SIGUSR2 for CPU emulation thread
  if (pthread_sigmask(SIG_UNBLOCK, &blomcall_usr2sigset, NULL) == -1) {
    perror("pthread_sigmask");
    return 0;
  }

  // Install a FS or GS segment area
  if (!install_segment()) {
    return 0;
  }

  blomcall_enable = 1;

  return 1;
}


void blomcall_destroy(void) {
  struct sigaction sa;
  stack_t ss;

  // Delete timer
  timer_delete(blomcall_timer);

  // Unblock signals (we will probably get a SIGUSR1 here!)
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);
  pthread_sigmask(SIG_UNBLOCK, &blomcall_usr1sigset, NULL);
  pthread_sigmask(SIG_UNBLOCK, &blomcall_usr2sigset, NULL);

  // Restore signal handlers
  sa.sa_handler = SIG_DFL;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGUSR1, &sa, NULL);
  sigaction(SIGUSR2, &sa, NULL);

  // Disable and restore signal stack to the default
  ss.ss_sp = NULL;
  ss.ss_flags = SS_DISABLE;
  ss.ss_size = 0;

  sigaltstack(&ss, NULL);
  free(sighandler_stack);
}

/*** Opcode handling *********************************************************/

unsigned long REGPARAM2 blomcall_qops (uae_u32 opcode, struct regstruct* regs) {
  uae_u32 rts_pc = get_long(m68k_areg(regs, 7));
  void* newpc = (void*) (uintptr_t) get_long(m68k_getpc(regs) + 2);
  uae_u32 rc;

  // Sanity check
  if (!blomcall_enable) {
    return op_illg (opcode, regs);
  }

  // Pop return address
  m68k_areg(regs, 7) += 4;

#if 1
  blomcall_func* fn = (blomcall_func*) newpc;

  rc = fn(regs->regs, regs->fp);

#else

#if defined (__i386__)
  __asm__ volatile ("			\n\
		mov   %%esp,%%esi	\n\
		mov   %1,%%esp		\n\
		call  *%4		\n\
		mov   %%esi,%%esp"
		    : "=a" (rc) :
		      "r" /* 1 */ (m68k_areg(regs, 7)),
		      "a" /* 2 */ (regs->regs),
		      "d" /* 3 */ (regs->fp),
		      "r" /* 4 */ (newpc) :
		      "cc", /* "memory", */ /* "eax", */ "ecx" /*, "edx" */, "esi" );

#elif defined (__x86_64__)
  __asm__ volatile ("			\n\
		mov   %%rsp,%%r12	\n\
		mov   %1,%%esp		\n\
		call  *%4		\n\
		mov   %%r12,%%rsp"
		    : "=a" (rc) :
		      "r" /* 1 */ (m68k_areg(regs, 7)),
		      "D" /* 2 */ (regs->regs),
		      "S" /* 3 */ (regs->fp),
		      "r" /* 4 */ (newpc) :
		      "cc", /* "memory",  */"rcx", "rdx", /* "rsi", "rdi", */
		      "r8", "r9", "r10", "r11", "r12");

#else
# error Unsupported architecture!
#endif

#endif

  if (opcode == OP_BJMPQ) {
    m68k_dreg (regs, 0) = rc;          // eax -> d0
  }

  m68k_setpc(regs, rts_pc);
  fill_prefetch_slow(regs);
  return 4;
}

unsigned long REGPARAM2 blomcall_ops (uae_u32 opcode, struct regstruct* regs) {
  static unsigned long remaining_cycles = 0;
  static uae_u64 start_time = 0;
  unsigned long cycles;
  uae_u64 call_time;
  uae_u8* ix86addr;

  cycles = limited_cycles(remaining_cycles);
  if (cycles != 0) {
    remaining_cycles -= cycles;
    return cycles;
  }

  // Sanity check
  if (!blomcall_enable) {
    return op_illg (opcode, regs);
  }

  switch (opcode) {
    case OP_BJMP:
    case OP_BJMPNR: {
      start_time = read_processor_time ();

      uae_u32 bcctx;

      // TODO: Cache and reuse used contexts
      blomcall_ctx = malloc (sizeof (struct blomcall_context));

      blomcall_ctx->op_resume         = bswap_16(OP_BRESUME);
      blomcall_ctx->saved_stack_bytes = 0;
      blomcall_ctx->magic             = BRESUME_MAGIC;
      blomcall_ctx->rts_pc            = get_long(m68k_areg(regs, 7));
      blomcall_ctx->rts_a7            = m68k_areg(regs, 7) + 4;
      blomcall_ctx->regs              = regs;

      m68k_areg(regs, 7) = blomcall_ctx->rts_a7;

      // Make sure the emulation can access this address
      assert ((((uae_u64) (uintptr_t) blomcall_ctx) & 0xffffffff00000000ULL) == 0);

      bcctx = ((uae_u32) (uintptr_t) blomcall_ctx) & 0xffff0000;
      put_mem_bank (bcctx, &directmem_bank, 0);
      break;
    }

    case OP_BRESUME: {
      start_time = read_processor_time ();

      blomcall_ctx = (void*) (uintptr_t) m68k_getpc(regs);

      assert (blomcall_ctx->magic == BRESUME_MAGIC &&
	      blomcall_ctx->saved_stack_bytes <= sizeof (blomcall_ctx->saved_stack));
      break;
    }

    case OP_BJMPQ:
    case OP_BJMPNRQ:
    default:
      // Something is really wrong if we get here
      abort();
  }

  switch (sigsetjmp(blomcall_ctx->emuljmp, 0)) {
    case 0: {
      // m68k code executed blomcall trap

      if (opcode == OP_BRESUME) {
	if (blomcall_ctx->saved_stack_bytes == 0) {
	  // Go back after timer break
	  pthread_kill(pthread_self(), SIGUSR2);
	}
	else {
	  // Restore stack frame contents and go back after m68k call
//	  uae_u8* sp = (uae_u8*) get_jmpbuf_sp(&blomcall_ctx->m68kjmp) - RED_ZONE_SIZE;
	  uae_u8* sp = (uae_u8*) blomcall_ctx->stack_pointer - RED_ZONE_SIZE;

	  memcpy(sp, blomcall_ctx->saved_stack, blomcall_ctx->saved_stack_bytes);
	  siglongjmp(blomcall_ctx->m68kjmp, 1);
	}
      }
      else {
	void* newpc = (void*) (uintptr_t) get_long(m68k_getpc(blomcall_ctx->regs) + 2);
	void* retpc = (opcode == OP_BJMP ?
		       (void*) blomcall_exit :
		       (void*) blomcall_exitnr);

	assert ((((uae_u64) (uintptr_t) retpc) & 0xffffffff00000000ULL) == 0);

	m68k_setpc(blomcall_ctx->regs, (uae_u32) (uintptr_t) blomcall_ctx);
	fill_prefetch_slow(blomcall_ctx->regs);

	// Don't do this until we have reloaded the stack pointer:
	// pthread_sigmask(SIG_UNBLOCK, &blomcall_usr1sigset, NULL);
	// Also note that the only memory constraint allowed is the stack
	// argument; after the new stack pointer is set, memory arguments will
	// most likely be wrong!

#if defined (__i386__)
	__asm__ volatile ("		\n\
		mov   %0,%%esp		\n\
		push  %6		\n\
		push  $0		\n\
		push  %1		\n\
		push  %2		\n\
		call  pthread_sigmask	\n\
		addl  $12,%%esp		\n\
		mov   %3,%%eax		\n\
		mov   %4,%%edx		\n\
		jmp   *%5"
			      : :
				"m" /* 0 */ (m68k_areg(blomcall_ctx->regs, 7)),
				"i" /* 1 */ (&blomcall_usr1sigset),
				"i" /* 2 */ (SIG_UNBLOCK),
				"r" /* 3 */ (blomcall_ctx->regs->regs),
				"r" /* 4 */ (blomcall_ctx->regs->fp),
				"r" /* 5 */ (newpc),
				"r" /* 6 */ (retpc) :
				"memory", "eax", "ecx", "edx");

#elif defined (__x86_64__)
	__asm__ volatile ("		\n\
		mov   %0,%%esp		\n\
		push  %6		\n\
		xor   %%edx,%%edx	\n\
		mov   %1,%%esi		\n\
		mov   %2,%%edi		\n\
		call  pthread_sigmask	\n\
		mov   %3,%%rdi		\n\
		mov   %4,%%rsi		\n\
		jmp   *%5"
			      : :
				"m" /* 0 */ (m68k_areg(blomcall_ctx->regs, 7)),
				"i" /* 1 */ (&blomcall_usr1sigset),
				"i" /* 2 */ (SIG_UNBLOCK),
				"r" /* 3 */ (blomcall_ctx->regs->regs),
				"r" /* 4 */ (blomcall_ctx->regs->fp),
				"r" /* 5 */ (newpc),
				"r" /* 6 */ (retpc) :
				"memory", "rax", "rcx", "rdx", "rdi", "rsi",
				"r8", "r9", "r10", "r11");

#else
# error Unsupported architecture!
#endif
      }
      
      // Never reached
      abort();
    }

    case 1: {
      // Native code returned (via blomcall_exit()/blomcall_exitnr())

      m68k_areg(blomcall_ctx->regs, 7) = blomcall_ctx->rts_a7;
      m68k_setpc (blomcall_ctx->regs, blomcall_ctx->rts_pc);
      fill_prefetch_slow(blomcall_ctx->regs);
      
      free(blomcall_ctx);
      break;
    }

    case 2: {
      // Native code interrupted by timer

      assert ((((uae_u64) get_sigctx_sp(blomcall_ctx))
	       & 0xffffffff00000000ULL) == 0);

      m68k_areg(blomcall_ctx->regs, 7) =
	(uae_u32) get_sigctx_sp(blomcall_ctx) - RED_ZONE_SIZE;
      blomcall_ctx->saved_stack_bytes = 0;
      break;
    }

    case 3: {
      // Native code called blomcall_callfunc68k()
      
      uae_u8* a7 = (uae_u8*) (uintptr_t) m68k_areg(blomcall_ctx->regs, 7);
//      uae_u8* sp = (uae_u8*) get_jmpbuf_sp(&blomcall_ctx->m68kjmp) - RED_ZONE_SIZE;
      uae_u8* sp = (uae_u8*) blomcall_ctx->stack_pointer - RED_ZONE_SIZE;

      // a7 has been set up by the assembly code and
      // __blomcall_callfunc68k() so it's the same as the native stack
      // pointer at the time just before blomcall_callfunc68k() was
      // called.
      //
      // sp (which is below a7) is native stack pointer inside
      // sigsetjmp(m68kjmp). Since we were running on the m68k stack,
      // it's always in the 32-bit address space.
      //
      // The m68k PC is already setup.
      //
      // What we need to do now is to push the address of blomcall_ctx
      // (which begins with a BRESUME opcode) so that when the m68k
      // executes an 'rts', it finds an BRESUME instruction and
      // siglongjmps us back to __blomcall_callfunc68k().
      //
      // However, first we must save the stack frame between sp and a7
      // so we can safely return back to __blomcall_callfunc68k. It's
      // quite likely that the m68k code will destroy this area
      // otherwise! At least we will, when pusing the address to
      // BRESUME.

      assert ((size_t) (a7 - sp) <= sizeof (blomcall_ctx->saved_stack));
      assert ((((uae_u64) (uintptr_t) blomcall_ctx) & 0xffffffff00000000ULL) == 0);

      memcpy(blomcall_ctx->saved_stack, sp, (a7 - sp));
      blomcall_ctx->saved_stack_bytes = (size_t) (a7 - sp);

      m68k_areg(blomcall_ctx->regs, 7) -= 4;
      put_long(m68k_areg(blomcall_ctx->regs, 7), (uae_u32) (uintptr_t) blomcall_ctx);
      break;
    }    

    default:
      // Never reached
      abort();
  }

  // BJMP/BRESUME not active at this point
  blomcall_ctx = NULL;

  call_time = read_processor_time () - start_time;

  remaining_cycles = (unsigned long) ((double) call_time / 
				      syncbase * 7.16e6 * 1.2 /* lie some */);

/*   { */
/*     static int c = 0; */
    
/*     ++c; */
/*     if ((c % 10) == 0) { */
/*       printf("c: %d, bc_cnt: %ld, call_time: %ld, syncbase: %ld, cycles; %ld     \r", */
/* 	     c, bc_cnt / 1000000, call_time, syncbase, remaining_cycles); */
/*     } */
/*   } */

  cycles = limited_cycles(remaining_cycles);
  remaining_cycles -= cycles;
  return cycles;
}

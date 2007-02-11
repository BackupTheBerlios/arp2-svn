
enum {
  OP_BJMP    = 0xfe00,
  OP_BJMPQ   = 0xfe01,
  OP_BJMPNR  = 0xfe05,
  OP_BJMPNRQ = 0xfe06,

  OP_BRESUME = 0xffff
};


#ifdef BLOMCALL

#include <setjmp.h>
#include <signal.h>
#include <sys/ucontext.h>

#if defined(__linux__)
# define ucontext kernel_ucontext /* argh!! */
# include <asm/ucontext.h>
#else
# define kernel_ucontext ucontext /* assume libc's and kernel's ucontext match */
#endif


#include "machdep/machdep.h"

#ifndef RED_ZONE_SIZE
#define RED_ZONE_SIZE 0
#endif


#define BRESUME_MAGIC 0x42526573756d6530ULL // 'BResume0'

struct blomcall_context {
    uae_u16           op_resume;
    uae_u16           saved_stack_bytes;
    uae_u32	      rts_pc;
    uae_u32	      rts_a7;
    uae_u32	      saved_errno;
    struct regstruct* regs;
    uae_u64           magic;
    uae_u32           saved_regs[16];
    fptype            saved_fpregs[8];
    uae_u8            saved_stack[128+RED_ZONE_SIZE];
    void*             stack_pointer;
    struct kernel_ucontext uc;
    struct _fpstate        fp;
    sigjmp_buf        emuljmp;
    sigjmp_buf        m68kjmp;
};

struct blomcall_segment {
    uae_u32 length;
    uae_u32 execbase;
    uae_u32 userdata;
    uae_u32 callfunc;
    uae_u32 calllib;
};


int blomcall_init (void);
void blomcall_destroy(void);
int blomcall_reset(void);
unsigned long blomcall_qops(uae_u32 opcode, struct regstruct *regs) REGPARAM;
unsigned long blomcall_ops(uae_u32 opcode, struct regstruct *regs) REGPARAM;

uae_u32 blomcall_calllib68k(uae_u32 args[16], fptype fp[8], uae_u32 offset) REGPARAM;
uae_u32 blomcall_callfunc68k(uae_u32 args[16], fptype fp[8], uae_u32 addr) REGPARAM;

typedef uae_u32 blomcall_func(uae_u32 args[16], fptype fp[8]) REGPARAM;

#else
# define blomcall_qops(opcode, regs) op_illg(opcode, regs)
# define blomcall_ops(opcode, regs) op_illg(opcode, regs)
#endif

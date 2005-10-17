
#include <signal.h>
#include <setjmp.h>
//#include <ucontext.h>
#include <asm/sigcontext.h>

enum {
  OP_BJMP   = 0xff00,
  OP_BJMPNR = 0xff05,
  OP_BRESUME = 0xffff
};

struct blomcall_context {
    struct sigcontext sc;
    struct _fpstate   fp;
//    volatile unsigned long disable;
    sigjmp_buf        emuljmp;
    uae_u32           pc;
    uae_u32           a7;
    uae_u32*          real_a7;
    uae_u32	      rts_pc;
};

struct blomcall_stack {
    uae_u16                  op_resume;
    uae_u16                  zero;
    struct blomcall_context* context;
};

int blomcall_init (void);
unsigned long blomcall_ops(uae_u32 opcode);

#ifndef BLOMCALL
# define blomcall_ops(opcode) op_illg(opcode)
#endif

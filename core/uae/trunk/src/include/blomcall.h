
#include <signal.h>
#include <setjmp.h>
#include <ucontext.h>

enum {
  OP_BCALL   = 0xff00,
  OP_BCALLNR = 0xff05,
  OP_BRESUME = 0xffff
};

struct blomcall_context {
    unsigned long eax;
    unsigned long edx;
    unsigned long ecx;
    unsigned long ebx;
    unsigned long eip;
    unsigned long eflags;
    volatile unsigned long disable;
    ucontext_t    ix86context;
    sigjmp_buf    emuljmp;
};

struct blomcall_stack {
    uae_u16                  op_resume;
    uae_u16                  zero;
    uae_u32                  pc;
    uae_u32                  a7;
    struct blomcall_context* context;
};

unsigned long blomcall_ops(uae_u32 opcode);

extern int in_blomcall;

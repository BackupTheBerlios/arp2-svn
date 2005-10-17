 /*
  * UAE - The Un*x Amiga Emulator
  *
  * AMD64/GCC stack magic definitions for autoconf.c
  *
  * Copyright 2005 Richard Drummond
  */

#include <setjmp.h>

STATIC_INLINE void transfer_control (void *, int, void *, void *, int) __attribute__((noreturn));

#define CAN_DO_STACK_MAGIC

struct stack_frame
{
    uae_u64 return_addr;

    /* Local area */
    uae_u32 local_has_retval;
    uae_u32 local_retval;
};

STATIC_INLINE void transfer_control (void *s, int size, void *pc, void *f, int has_retval)
{
    struct stack_frame *stacktop = (struct stack_frame *)((char *)s + size - sizeof (struct stack_frame));

    stacktop->local_retval     = 0;
    stacktop->local_has_retval = has_retval;
    stacktop->return_addr      = 0xC0DEDBADC0DEDBAD;

    __asm__ __volatile__ ("\
	mov   %1, %%rsp   \n\
	mov   %2, %%rdi   \n\
	mov   %3, %%rsi   \n\
	mov   %4, %%rdx   \n\
	jmp   *%0"
	:
	: "r" (pc),
	  "r" (stacktop),
	  "r" (s),
	  "r" (f),
	  "r" (&(stacktop->local_retval))
	: "memory", "rsp", "rdi", "rsi", "rdx");

       /* Not reached. */
       abort ();
}

STATIC_INLINE uae_u32 get_retval_from_stack (void *s, int size)
{
    return ((struct stack_frame *)((char *)s + size - sizeof(struct stack_frame)))->local_retval;
}

STATIC_INLINE int stack_has_retval (void *s, int size)
{
    return ((struct stack_frame *)((char *)s + size - sizeof(struct stack_frame)))->local_has_retval;
}

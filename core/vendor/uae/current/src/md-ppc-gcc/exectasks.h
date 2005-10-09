 /*
  * UAE - The Un*x Amiga Emulator
  *
  * PPC-SYSV/GCC and PPC/Mach-O stack magic definitions for autoconf.c
  *
  * Copyright 2004-2005 Richard Drummond
  */

#include <setjmp.h>

#undef CAN_DO_STACK_MAGIC

#if (__GNUC__ > 2 || __GNUC_MINOR__ >= 7)

static inline void transfer_control (void *, int, void *, void *, int) __attribute__((noreturn));

#define CAN_DO_STACK_MAGIC
#define USE_EXECLIB

#ifdef __APPLE__

/* Mach-O stack frame */
struct stack_frame
{
    /* Linkage area - 6 words */
    uae_u32 back_chain;
    uae_u32 cr_save;
    uae_u32 lr_save;
    uae_u32 linkage_reserved[3];

    /* Output parameter area - need at least 8 words */
    uae_u32 parameters[8];

    /* Local area */
    uae_u32 local_has_retval;
    uae_u32 local_retval;

    /* Previous frame */
    uae_u32 end_back_chain;

    /* Padding for alignment */
    uae_u32 padding[3];
};

# define R1 "r1"
# define R2 "r2"
# define R3 "r3"
# define R4 "r4"
# define R5 "r5"

#else

/* SYSV stack frame */
struct stack_frame
{
    /* Linkage area */
    uae_u32 back_chain;
    uae_u32 lr_save;

    /* Local area */
    uae_u32 local_has_retval;
    uae_u32 local_retval;

    /* Previous frame */
    uae_u32 end_back_chain;

    /* Padding for alignment */
    uae_u32 padding[3];
};

# define R1  "1"
# define R2  "2"
# define R3  "3"
# define R4  "4"
# define R5  "5"
#endif

STATIC_INLINE void transfer_control (void *s, int size, void *pc, void *f, int has_retval)
{
    struct stack_frame *stacktop = (struct stack_frame *)((char *)s + size - sizeof (struct stack_frame));

    stacktop->end_back_chain   = 0xC0DEDBAD;
    stacktop->local_retval     = 0;
    stacktop->local_has_retval = has_retval;
    stacktop->back_chain       = (uae_u32) &stacktop->end_back_chain;

    __asm__ __volatile__ ("\
	mtctr %0        \n\
	mr    "R1",%1   \n\
	mr    "R3",%2   \n\
	mr    "R4",%3   \n\
	mr    "R5",%4   \n\
	bctr"
	:
	: "r" (pc),
	  "r" (stacktop),
	  "r" (s),
	  "r" (f),
	  "r" (&(stacktop->local_retval))
	: "memory", "r1", "r3", "r4", "r5", "cr2", "ctr");

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

#endif

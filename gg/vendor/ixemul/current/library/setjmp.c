/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)setjmp.s    5.1 (Berkeley) 5/12/90"
#endif /* LIBC_SCCS and not lint */

/*
 * C library -- setjmp, longjmp
 *
 *      longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *      setjmp(a)
 * by restoring registers from the stack,
 * and a struct sigcontext, see <signal.h>
 */

#define _KERNEL
#include "ixemul.h"
#include "defs.h"
#include "ix_internals.h"

#include <sys/syscall.h>

#ifdef NATIVE_MORPHOS

/* The __obsolete_XXX_setjmp functions are called by 68k
 * programs compiled with old 68k _JBLEN.
 * Allocate a bigger buffer, store a pointer to it in the
 * 68k buffer, and set the 'cr' field to 0, so that longjmp
 * can recognize that it is given a 68k buffer.
 */

int __obsolete_sigsetjmp();
int __obsolete__setjmp();
int __obsolete_setjmp();

static int _trampoline___obsolete_sigsetjmp();
static int _trampoline___obsolete__setjmp();
static int _trampoline___obsolete_setjmp();
static int _trampoline_sigsetjmp();
static int _trampoline__setjmp();
static int _trampoline_setjmp();
static int _trampoline__clean_longjmp();
static int _trampoline_siglongjmp();
static int _trampoline__longjmp();
static int _trampoline_longjmp();

struct EmulLibEntry _gate___obsolete_sigsetjmp = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline___obsolete_sigsetjmp
};

struct EmulLibEntry _gate___obsolete__setjmp = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline___obsolete__setjmp
};

struct EmulLibEntry _gate___obsolete_setjmp = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline___obsolete_setjmp
};

struct EmulLibEntry _gate_sigsetjmp = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline_sigsetjmp
};

struct EmulLibEntry _gate__setjmp = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline__setjmp
};

struct EmulLibEntry _gate_setjmp = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline_setjmp
};

struct EmulLibEntry _gate__clean_longjmp = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline__clean_longjmp
};

struct EmulLibEntry _gate_siglongjmp = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline_siglongjmp
};

struct EmulLibEntry _gate__longjmp = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline__longjmp
};

struct EmulLibEntry _gate_longjmp = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline_longjmp
};


asm(" .section \".text\"");

ENTRY(_trampoline___obsolete_sigsetjmp)
asm("
	lwz     5,60(2)
	lwz     3,4(5)
	lwz     4,8(5)
	b       __obsolete_sigsetjmp
");

ENTRY(__obsolete_sigsetjmp)
asm("
	cmpi    0,4,0
	stw     4,0(3)
	addi    3,3,4
	bne     __obsolete_setjmp
	b       __obsolete__setjmp
");


ENTRY(_trampoline___obsolete__setjmp)
asm("
	lwz     5,60(2)
	lwz     3,4(5)
	b       __obsolete__setjmp
");

ENTRY(__obsolete__setjmp)
asm("
	li      6,0             /* dummy signal mask */
	li      7,0             /*  and onstack */
	b       __obsolete_setup_setjmp
");


ENTRY(_trampoline___obsolete_setjmp)
asm("
	lwz     5,60(2)
	lwz     3,4(5)
	b       __obsolete_setjmp
");

ENTRY(__obsolete_setjmp)
asm("
__obsolete_setjmp:
	stwu    1,-16(1)
	mflr    0
	stw     0,20(1)
	bl      get_onstack_sigmask
	lwz     0,20(1)
	mtlr    0
	addi    1,1,16

__obsolete_setup_setjmp:
	/* r3 = save area, r6 = onstack, r7 = sigmask */
	stwu    1,-80(1)
	mflr    0
	stmw    18,16(1)	/* save d2-a6, which are clobbered */
	stw     0,84(1)		/* by a call to EmulCallOS() in malloc() */
	stw     3,8(1)
	stw     6,0(3)
	stw     7,4(3)
	lmw     18,8(2)
	li      3,376
	bl      malloc
	cmpi    0,3,0
	lwz     4,8(1)
	bne     ok4
	b       abort
ok4:
	stmw    18,8(2)
	li      5,0
	lwz     0,84(1)
	stw     3,8(4)
	lmw     18,16(1)
	lwz     6,0(4)
	mtlr    0
	lwz     7,4(4)
	stw     5,24(4)
	addi    1,1,80
	b       setup_setjmp
");


ENTRY(_trampoline_sigsetjmp)
asm("
	lwz     5,60(2)
	lwz     4,8(5)
	lwz     3,4(5)
	b       sigsetjmp
");

ENTRY(sigsetjmp)
asm("
	cmpi    0,4,0
	stw     4,0(3)
	addi    3,3,4
	bne     setjmp
	b       _setjmp
");


ENTRY(_trampoline__setjmp)
asm("
	lwz     5,60(2)
	lwz     3,4(5)
	li      6,0             /* dummy signal mask */
	li      7,0             /*  and onstack */
	b       setup_setjmp
");

ENTRY(_setjmp)
asm("
	li      6,0             /* dummy signal mask */
	li      7,0             /*  and onstack */
	b       setup_setjmp
");

asm("
    /* routine to obtain current values of onstack/sigmask */
    /* Attn: don't change r3/r4, return in r6/r7 */
get_onstack_sigmask:
	stwu    1,-32(1)        /* space for sigstack args/rvals */
	mflr    0
	lwz	5,56(2)		/* save a6, changed by the Forbid() call in sigblock */
	stw     3,16(1)
	stw     4,20(1)
	stw     0,36(1)
	stw	5,24(1)
	li      3,0             /* don't change it */
	addi    4,1,8           /* ...but return the current val */
	bl      sigstack        /* note: onstack returned in 12(1) */
	li      3,0             /* don't change mask, just return */
	bl      sigblock        /*   old value */
	lwz     0,36(1)
	lwz     6,12(1)         /* old onstack value */
	mr      7,3             /* sigmask */
	mtlr    0
	lwz	5,24(1)
	lwz     4,20(1)         /* r4 unchanged */
	lwz     3,16(1)         /* r3 unchanged */
	stw	5,56(2)		/* restore a6 */
	addi    1,1,32
	blr                     /* r7 = sigmask, r6 = onstack */
");

ENTRY(_trampoline_setjmp)
asm("
	lwz     5,60(2)
	lwz     3,4(5)
	lwz     4,8(5)
	b       setjmp
");

ENTRY(setjmp)
asm("
	stwu    1,-16(1)
	mflr    0
	stw     0,20(1)
	bl      get_onstack_sigmask
	lwz     0,20(1)
	addi    1,1,16
	mtlr    0

setup_setjmp:
	/* r3 = save area, r6 = onstack, r7 = sigmask */
	lis     4,SysBase@ha
	mr      8,1             /* r8  = r1 */
	lwz     4,SysBase@l(4)
	mr      9,2             /* r9  = r2 */
	lhz     10,0x126(4)     /* r10 = TDNestCnt, IDNestCnt */
	mfcr    12              /* r12 = cr */
	mflr    11              /* r11 = lr */
	stfd    14,104(3)
	stfd    15,112(3)
	ori     12,12,1         /* make cr !=0 */
	stfd    16,120(3)
	stfd    17,128(3)
	stfd    18,136(3)
	stfd    19,144(3)
	stmw    6,0(3)
	stfd    20,152(3)
	stfd    21,160(3)
	stfd    22,168(3)
	stfd    23,176(3)
	stfd    24,184(3)
	stfd    25,192(3)
	stfd    26,200(3)
	stfd    27,208(3)
	stfd    28,216(3)
	stfd    29,224(3)
	stfd    30,232(3)
	stfd    31,240(3)
	lmw     26,8(2)         /* d2-d7 */
	lwz     4,64(2)         /* pc */
	stmw    26,256(3)
	lmw     26,40(2)        /* a2-a7 */
	stw     4,248(3)
	stmw    26,280(3)
	lwz     6,0(31)         /* *a7 */
	lmw     14,0x88(2)      /* fp2-fp7 */
	stw     6,252(3)
	stmw    14,304(3)
	lmw     14,32(3)        /* restore r14-r31 */
	li      3,0             /* return 0 */
	stw     3,0(2)          /* in r3 and d0 */
	blr
");

ENTRY(_trampoline__clean_longjmp)
asm("
	lwz     5,60(2)
	lwz     3,4(5)
	lwz     4,8(5)
	b       _clean_longjmp
");

/* _clean_longjmp, do NOT change onstack/sigmask, do NOT change stackframe */
ENTRY(_clean_longjmp)
asm("
	lwz     5,24(3)         /* old style buffer ? */
	cmpi    0,5,0
	bne+    ok5
	lwz     3,8(3)          /* yes -> get the new one */
ok5:
	stwu    1,-16(1)
	mflr    0
	stw     0,20(1)
	bl      get_onstack_sigmask
	lwz     0,20(1)
	mtlr    0
	addi    1,1,16

	lwz     5,8(3)          /* load sp */
	stw     6,0(3)          /* save current onstack value */
	cmpi    0,5,0           /* ensure non-zero SP */
	stw     7,4(3)          /* save current signal mask */
	cmpi    1,4,0           /* test return value */
	beq-    botch           /* bad stack ! */
	bne+    1,ok2           /* return value = non-zero ok */
	li      4,1             /* else make non-zero */
	b       ok2
");


ENTRY(_trampoline_siglongjmp)
asm("
	lwz     5,60(2)
	lwz     3,4(5)
	lwz     4,8(5)
	b       siglongjmp
");

ENTRY(siglongjmp)
asm("
	lwz     5,24(3)         /* old style buffer ? */
	cmpi    0,5,0
	bne     ok6
	lwz     3,8(3)          /* yes -> get the new one */
ok6:
	lwz     5,0(3)
	addi    3,3,4
	cmpi    0,5,0
	bne     longjmp
	b       _longjmp
");

ENTRY(_trampoline__longjmp)
asm("
	lwz     5,60(2)
	lwz     3,4(5)
	lwz     4,8(5)
	b       _longjmp
");

/* _longjmp, do NOT change onstack/sigmask */
ENTRY(_longjmp)
asm("
	lwz     5,24(3)         /* old style buffer ? */
	cmpi    0,5,0
	bne+    ok7
	lwz     3,8(3)          /* yes -> get the new one */
ok7:
	stwu    1,-16(1)
	mflr    0
	stw     0,20(1)
	bl      get_onstack_sigmask
	lwz     0,20(1)
	addi    1,1,16
	mtlr    0

	stw     6,0(3)          /* save current onstack value */
	stw     7,4(3)          /* save current signal mask */
	b       longjmp
");

ENTRY(_trampoline_longjmp)
asm("
	lwz     5,60(2)
	lwz     3,4(5)
	lwz     4,8(5)
	b       longjmp
");

ENTRY(longjmp)
asm("
	lwz     5,24(3)         /* old style buffer ? */
	cmpi    0,5,0
	bne     ok8
	lwz     3,8(3)          /* yes -> get the new one */
ok8:
	stwu	1,-16(1)
        stw	3,8(1)
	stw	4,12(1)
	lwz     3,300(3)	/* r3 = new A7 */
	lwz	4,60(2)		/* r4 = old A7 */
	bl	stkrst68k	/* Go to the correct 68k stackframe */
	lwz	3,8(1)
	lwz	4,12(1)
	addi	1,1,8

	lwz     5,8(3)
	cmpi    0,5,0           /* ensure non-zero SP */
	cmpi    1,4,0           /* test return value */
	beq-    botch           /* oops! */
	bne+    1,ok1
	li      4,1             /* make return value non zero */
ok1:
	bl      __stkrst        /* Go to correct stackframe, r5 should */
				/* contain the SP */
ok2:
	lwz     2,12(3)
	lmw     14,304(3)       /* fp2-fp7 */
	lwz     6,252(3)        /* *a7 */
	stmw    14,0x88(2)
	lmw     26,280(3)       /* a2-a7 */
	lwz     5,248(3)        /* pc */
	stmw    26,40(2)
	stw     6,0(31)
	lmw     26,256(3)       /* d2-d7 */
	stw     5,64(2)
	stmw    26,8(2)
	lmw     12,24(3)
	lfd     14,104(3)
	lfd     15,112(3)
	lfd     16,120(3)
	lfd     17,128(3)
	lfd     18,136(3)
	lfd     19,144(3)
	lfd     20,152(3)
	lfd     21,160(3)
	lfd     22,168(3)
	lfd     23,176(3)
	lfd     24,184(3)
	lfd     25,192(3)
	lfd     26,200(3)
	lfd     27,208(3)
	lfd     28,216(3)
	lfd     29,224(3)
	lfd     30,232(3)
	lfd     31,240(3)
	mtcr    12
	b       sigreturn

botch:
	b       longjmperror
");

#else /* NATIVE_MORPHOS */

asm(".text");

ENTRY(__obsolete_sigsetjmp)
ENTRY(sigsetjmp)
asm("
	movl    sp@(4),a0
	movl    sp@(8),a0@
	addl    #4,a0
	movl    a0,sp@(4)
	tstl    a0@(-4)
	jne     _setjmp
");

ENTRY(__obsolete__setjmp)
ENTRY(_setjmp)
asm("
	moveq   #0,d0           /* dummy signal mask */
	movel   d0,d1           /*  and onstack */
	jra     setup_setjmp
");



asm("
	/* routine to obtain current values of onstack/sigmask */
get_onstack_sigmask:
	subql   #8,sp           /* space for sigstack args/rvals */
	clrl    sp@             /* don't change it... */
	movl    sp,sp@(4)       /* ...but return the current val */
	jbsr    _sigstack       /* note: onstack returned in sp@(4) */
	clrl    sp@             /* don't change mask, just return */
	jbsr    _sigblock       /*   old value */
	movl    sp@(4),d1       /* old onstack value */
	addql   #8,sp
	rts                     /* d0 = sigmask, d1 = onstack */
");

#define _MAKESTR(o) #o
#define MAKESTR(o) _MAKESTR(o)

ENTRY(__obsolete_setjmp)
ENTRY(setjmp)
asm("
	jsr     get_onstack_sigmask

setup_setjmp:
	movl    sp@(4),a0       /* save area pointer */
	movl    d1,a0@+         /* save old onstack value */
	movl    d0,a0@+         /* save old signal mask */
	lea     sp@(4),a1       /* adjust saved SP since we won't rts */
	movl    a1,a0@+         /* save old SP */
	movl    a5,a0@+         /* save old FP */
	movel   4:w,a1
	movew   a1@(0x126),a0@(2) /* use TDNestCnt and IDNestCnt from Sysbase ! */
	lea     a0@(4),a0       /* skip 4 bytes (the first two bytes used to */
				/*      contain task specific flags) */
	movl    sp@,a0@+        /* save old PC */
	clrl    a0@+            /* clean PS */
	moveml  d2-d7/a2-a4/a6,a0@      /* save remaining non-scratch regs */
	clrl    d0              /* return 0 */
	rts
");


/* _clean_longjmp, do NOT change onstack/sigmask, do NOT change stackframe */
/* Fixme: what to do with MorphOS ? restore r1 ? */
ENTRY(_clean_longjmp)
asm("
	jsr     get_onstack_sigmask
	movl    sp@(4),a0       /* get area pointer */
	movl    d1,a0@+         /* save current onstack value */
	movl    d0,a0@          /* save current signal mask */

	movl    sp@(4),a0       /* save area pointer */
	movel   a0@(8),d0       /* ensure non-zero SP */
	jeq     botch           /* oops! */
	movl    sp@(8),d1       /* grab return value */
	jne     ok2             /* non-zero ok */
	moveq   #1,d1           /* else make non-zero */
	jra     ok2
");

ENTRY(siglongjmp)
asm("
	movl    sp@(4),a0
	addl    #4,a0
	movl    a0,sp@(4)
	tstl    a0@(-4)
	jne     _longjmp
");

/* _longjmp, do NOT change onstack/sigmask */
ENTRY(_longjmp)
asm("
	jsr     get_onstack_sigmask
	movl    sp@(4),a0       /* get area pointer */
	movl    d1,a0@+         /* save current onstack value */
	movl    d0,a0@          /* save current signal mask */
	/* fall through */
");


ENTRY(longjmp)
asm("
	movl    sp@(4),a0       /* save area pointer */
	movl    a0@(8),d0       /* ensure non-zero SP */
	jeq     botch           /* oops! */
	movl    sp@(8),d1       /* grab return value */
	jne     ok1             /* non-zero ok */
	moveq   #1,d1           /* else make non-zero */
ok1:
	jbsr    ___stkrst       /* Go to correct stackframe, d0 should */
				/* contain the SP */
ok2:
	movl    d1,d0
	moveml  a0@(28),d2-d7/a2-a4/a6  /* restore non-scratch regs */
	movl    a0,sp@-         /* let sigreturn */
	jbsr    _sigreturn      /*   finish for us */

botch:
	jsr     _longjmperror
	stop    #0
");

#endif /* NATIVE_MORPHOS */

/*
 * This routine is called from longjmp() when an error occurs.
 * Programs that wish to exit gracefully from this error may
 * write their own versions.
 */

void
longjmperror()
{
#define ERRMSG  "longjmp botch\n"
	syscall (SYS_write, 2, ERRMSG, sizeof(ERRMSG) - 1);
	syscall (SYS_abort);
}


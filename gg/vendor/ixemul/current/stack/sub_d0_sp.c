#include "a4.h"         /* for the A4 macro */

#ifndef __PPC__
asm("
	.text
	.even
	.globl  ___sub_d0_sp
	.globl  ___move_d0_sp
	.globl  ___unlk_a5_rts

___sub_d0_sp:
	movel   sp@+,a0
	movel   sp,d1
	subl    d0,d1
	cmpl    "A4(___stk_limit)",d1
	jcc     l0
	jbsr    ___stkext
l0:     subl    d0,sp
	jmp     a0@

___move_d0_sp:
	jra     ___stkrst

___unlk_a5_rts:
	movel   d0,a0
	movel   a5,d0
	jbsr    ___stkrst
	movel   a0,d0
	movel   sp@+,a5
	rts
");
#else
asm("
	.section \".text\"
	.align  2
	.type   __alloc_stk_ext,@function
	.type   __alloc_stk_chk,@function
	.globl  __alloc_stk_ext
	.globl  __alloc_stk_chk

/*  r3 = size */
__alloc_stk_chk: /* fix me */
__alloc_stk_ext:
"
#if defined(LBASEREL)
"
	addis   12,13,__stk_limit@drelha
	lwz     12,__stk_limit@drell(12)
"
#elif defined(BASEREL)
"
	lwz     12,__stk_limit@sdarel(13)
"
#else
"
	lis     12,__stk_limit@ha
	lwz     12,__stk_limit@l(12)
"
#endif
"
	sub     11,1,3
	lwz     0,0(1)
	cmp     0,12,11
	neg     4,3
	bge-    l1
	stwux   0,1,4
	blr
l1:     b       __stkext
__end__alloc_stk_ext:
	.size	__alloc_stk_ext,__end__alloc_stk_ext-__alloc_stk_ext
");

#endif


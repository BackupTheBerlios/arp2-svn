
#define _KERNEL
#include "ixemul.h"
#include "defs.h"
#include "ix_internals.h"

#define _MAKESTR(o) #o
#define MAKESTR(o) _MAKESTR(o)

ENTRY(sigreturn)
asm("
	lis     5,SysBase@ha
	lwz     7,0(3)                  /* r7  = onstack */
	lwz     5,SysBase@l(5)          /* r5  = SysBase */
	lwz     2,12(3)                 /* restore r2 */
	lwz     8,4(3)                  /* r8  = sigmask */
	lwz     6,0x114(5)              /* r6  = ThisTask */
	lwz     10,16(3)                /* r10 = Flags */
	lwz     9,"MAKESTR(USERPTR_OFFSET)"(6)/* r9  = u_ptr */
	lwz     11,20(3)                /* r11 = lr */
	sth     10,0x126(5)             /* restore TDNestCnt/IDNestCnt */
	stw     7,"MAKESTR(U_ONSTACK_OFFSET)"(9)/* restore u_onstack */
	mtlr    11                      /* restore lr */
	stw     8,"MAKESTR(P_SIGMASK_OFFSET)"(9)/* restore p_sigmask */
	lwz     1,8(3)                  /* restore r1 */
	stw     4,0(2)                  /* d0 = return value */
	mr      3,4                     /* r3 too */
	blr
");

ENTRY(resetfpu)
asm("
	mtfsfi 0,0
	mtfsfi 1,0
	mtfsfi 2,0
	mtfsfi 3,0
	mtfsfi 4,0
	mtfsfi 5,0
	mtfsfi 6,0
	mtfsfi 7,1
	blr
");


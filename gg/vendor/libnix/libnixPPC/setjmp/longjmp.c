#include <setjmp.h>

void longjmp(jmp_buf b,int r)
{
asm("
	lmw	9,0(3)
	mtlr	11
	mtcr	12
	mr	1,10
	mr	2,9
	mr.	3,4
	bnelr
	li	3,1
	blr
	");
}

#include <setjmp.h>

int setjmp(jmp_buf b)
{
asm("
	mflr	11
	mfcr	12
	mr	10,1
	mr	9,2
	stmw	9,0(3)
	li	3,0
	blr
	");
}

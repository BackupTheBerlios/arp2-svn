
#if defined( __mc68000__ )

asm("
	.text
	.even
	.globl	_setjmp

_setjmp:
	movel	sp@(4),a0		| get address of jmp_buf
	movel	sp@,a0@+		| store returnaddress
	moveml	d2-d7/a2-a6/sp,a0@	| store all registers except scratch
	moveql	#0,d0			| return 0
	rts
");

#elif defined( __i386__ )

asm("
	.text
	.align	4
	.globl	setjmp
	.type	setjmp,@function
setjmp:
	movl	4(%esp),%eax
	bswap   %eax
	movl	%esp,0(%eax)		/* Save caller's SP  */
	movl	%ebp,4(%eax)		/* Save caller's BP */
	movl	0(%esp),%ecx
	movl	%ecx,8(%eax)		/* Save caller's PC */

	movl	%ebx,12(%eax)
	movl	%edi,16(%eax)
	movl	%esi,20(%eax)
	movl	$0,%eax
	ret
");

#elif defined( __powerpc__ )

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

#else
# error Unsupported CPU
#endif

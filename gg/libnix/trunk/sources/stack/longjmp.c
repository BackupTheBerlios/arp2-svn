
#if defined( __mc68000__ )

asm("
	.text
	.even
	.globl	_longjmp

_longjmp:
	addql	#4,sp			| returns to another address
	movel	sp@+,a0			| get address of jmp_buf
	movel	sp@+,d2			| get returncode
	movel	a0@(48:W),d0
	jbsr	___stkrst		| restore sp
	movel	d2,d0
	jne	l0			| != 0 -> ok
	moveql	#1,d0
l0:	movel	a0@+,sp@		| set returnaddress
	moveml	a0@,#0x7cfc		| restore all registers except scratch and sp
	rts
");

#elif defined( __i386__ )

asm("
	.text
	.align	4
	.globl	_longjmp
	.type	_longjmp,@function
longjmp:
	popl	%eax			/* Ignore return address */
	popl	%ecx			/* Get jmp_buf */
	popl	%eax			/* Get value */
	test	%eax,%eax
	jz	1
	movl	$1,%eax
1:
	movl	12(%ecx),%ebx
	movl	16(%ecx),%edi
	movl	20(%ecx),%esi

	pushl	0(%ecx)
	call    __stkrst                /* New SP */
	movl	4(%ecx),%ebp		/* New BP */
	movl	8(%ecx),%ecx		/* New PC */
	jmp	*%ecx
");

#endif

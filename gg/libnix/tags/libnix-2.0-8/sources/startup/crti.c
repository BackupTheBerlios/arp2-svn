
#if defined (__i386__)

__asm( "
	.section .init

	.globl	_init
	.type	_init,@function
_init:
	push   %ebp
	mov    %esp,%ebp


	.section .fini

	.globl	_fini
	.type	_fini,@function
_fini:
	push   %ebp
	mov    %esp,%ebp
");

#elif defined (__powerpc__)

__asm("
	.section .init

	.globl	_init
	.type	_init,@function
_init:
	stwu	%r1,-16(%r1)
	mflr	%r0
	stw	%r0,12(%r1)


	.section .fini

	.globl	_fini
	.type	_fini,@function
_fini:
	stwu	%r1,-16(%r1)
	mflr	%r0
	stw	%r0,12(%r1)
");


#else
# error Unknown CPU
#endif

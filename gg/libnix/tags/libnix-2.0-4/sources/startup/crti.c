
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

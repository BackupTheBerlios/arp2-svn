
#if defined (__i386__)

__asm( "
	.section .init

	leave
	ret

	.section .fini

	leave
	ret
");

#elif defined (__powerpc__)

__asm( "
	.section .init

	lwz	%r0,12(%r1)
	mtlr	%r0
	la	%r1,16(%r1)
        blr

	.section .fini

	lwz	%r0,12(%r1)
	mtlr	%r0
	la	%r1,16(%r1)
        blr
");

#else
# error Unknown CPU
#endif


__asm( "
	.section .init

	leave
	ret

	.section .fini

	leave
	ret
");

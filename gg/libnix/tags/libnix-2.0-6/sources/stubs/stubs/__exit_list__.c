
#if defined( __ELF__ )

__asm("
	.section .libnix___EXIT_LIST__
	.globl	__EXIT_END__
__EXIT_END__:
	.long	0,0
");

#else

void *__EXIT_LIST__[2]={ 0,0 };

#endif

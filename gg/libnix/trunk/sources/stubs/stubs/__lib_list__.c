
#if defined( __ELF__ )

__asm("
	.section .libnix___LIB_LIST__
	.globl	__LIB_END__
__LIB_END__:
	.long	0
");

#else

void *__LIB_LIST__[2]={ 0,0 };

#endif

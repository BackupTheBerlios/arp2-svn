
#if defined( __ELF__ )

__asm("
	.section .libnix___INIT_LIST__
	.globl	__INIT_END__
__INIT_END__:
	.long	0,0
");

#else

void *__INIT_LIST__[2]={ 0,0 };

#endif

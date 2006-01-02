
__asm("	.int	init-@				\n\
	.int	init_end-init			\n\
	.int	acknowledge-@			\n\
	.int	acknowledge_end-acknowledge	\n\
	.int	release-@			\n\
	.int	release_end-release		\n\
");

int init() {}
__asm("init_end:");

int acknowledge() {}
__asm("acknowledge_end:");

int release() {}
__asm("release_end:");

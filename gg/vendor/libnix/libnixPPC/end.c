asm("
	.section \".init\",\"a\"
	.long 0,0
	");
asm("
	.section \".fini\",\"a\"
	.long 0,0
	");
asm("
	.section \".ctors\",\"a\"
	.long 0
	");
asm("
	.section \".dtors\",\"a\"
	.long 0
	");

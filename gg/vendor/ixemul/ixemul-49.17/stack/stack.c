#include "ixemul.h"

/* I wish I knew a more elegant way to do this:

   sstr(STACKSIZE) -> str((16384)) -> "(16384)"
*/
#define str(s) #s
#define sstr(s) str(s)

#ifndef __PPC__
asm("
	.data
	.even
	.globl  ___stack
	.ascii  \"StCk\"        | Magic cookie
___stack:
	.long   " sstr(STACKSIZE) "
	.ascii  \"sTcK\"        | Magic cookie
");
#else
asm("
	.section \".data\"
	.align  2
	.globl  __stack
	.ascii  \"StCk\"        /* Magic cookie */
__stack:
	.long   " sstr(STACKSIZE * 2) "
	.ascii  \"sTcK\"        /* Magic cookie */
");

#endif


#if defined( __mc68000__ )

asm("
		.text

		.globl	_HookEntry

_HookEntry:	movel	a1,sp@-
		movel	a2,sp@-
		movel	a0,sp@-
		movel	a0@(12:W),a0
		jsr	a0@
		lea	sp@(12:W),sp
		rts
");

#endif


#if defined( __mc68000__ )

#include "bases.h"

asm("
		.globl	___mulsi3

| D0 = D0 * D1

___mulsi3:	moveml	sp@(4),d0/d1
		movel	"A4(_UtilityBase)",a0
		jmp	a0@(-138:W)
");

#elif defined( __i386__ )

asm("
	.text
	.align	4
	.globl	__mulsi3
	.type	__mulsi3,@function
__mulsi3:
	push	%ebp
	mov	%esp,%ebp

	movl	12(%ebp),%eax
	bswap	%eax
	movl	8(%ebp),%ecx
	bswap	%ecx
	imull	%ecx,%eax

	leave
	ret
");

#else
# warning Unsupported CPU
#endif

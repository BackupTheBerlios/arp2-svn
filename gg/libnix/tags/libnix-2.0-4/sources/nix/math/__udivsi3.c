
#if defined( __mc68000__ )

#include "bases.h"

asm("
		.globl	___umodsi3
		.globl	___udivsi3

| D1.L = D0.L % D1.L unsigned

___umodsi3:	moveml	sp@(4:W),d0/d1
		jbsr	___udivsi4
		movel	d1,d0
		rts

| D0.L = D0.L / D1.L unsigned

___udivsi3:	moveml	sp@(4:W),d0/d1
___udivsi4:	movel	"A4(_UtilityBase)",a0
		jmp	a0@(-156:W)
");

#elif defined( __i386__ )

asm("
	.text
	.align	4
	.globl	__umodsi3
	.type	__umodsi3,@function
__umodsi3:
	push	%ebp
	mov	%esp,%ebp

	movl	8(%ebp),%eax
	bswap	%eax
	xorl	%edx,%edx
	movl	12(%ebp),%ecx
	bswap	%ecx
	divl	%ecx,%eax
	movl	%edx,%eax

	leave
	ret

	.align	4
	.globl	__udivsi3
	.type	__udivsi3,@function
__udivsi3:
	push	%ebp
	mov	%esp,%ebp

	movl	8(%ebp),%eax
	bswap	%eax
	xorl	%edx,%edx
	movl	12(%ebp),%ecx
	bswap	%ecx
	divl	%ecx,%eax

	leave
	ret
");

#else
# error Unsupported CPU
#endif


#if defined( __mc68000__ )

#include "bases.h"

asm("
		.globl	_div
		.globl	_ldiv
		.globl	___modsi3
		.globl	___divsi3

| D1.L = D0.L % D1.L signed

___modsi3:	moveml	sp@(4:W),d0/d1
		jbsr	___divsi4
		movel	d1,d0
		rts

| D0.L = D0.L / D1.L signed

_div:
_ldiv:
___divsi3:	moveml	sp@(4:W),d0/d1
___divsi4:	movel	"A4(_UtilityBase)",a0
		jmp	a0@(-150:W)
");

#elif defined( __i386__ )

asm("
	.text
	.align	4
	.globl	__modsi3
	.type	__modsi3,@function
__modsi3:
	push	%ebp
	mov	%esp,%ebp

	movl	8(%ebp),%eax
	bswap	%eax
	cltd
	movl	12(%ebp),%ecx
	bswap	%ecx
	idivl	%ecx,%eax
	movl	%edx,%eax

	leave
	ret

	.align	4
	.globl	div
	.globl	ldiv
	.globl	__divsi3
	.type	div,@function
	.type	ldiv,@function
	.type	__divsi3,@function
div:
ldiv:
__divsi3:
	push	%ebp
	mov	%esp,%ebp

	movl	8(%ebp),%eax
	bswap	%eax
	cltd
	movl	12(%ebp),%ecx
	bswap	%ecx
	idivl	%ecx,%eax

	leave
	ret
");

#else
# error Unsupported CPU
#endif

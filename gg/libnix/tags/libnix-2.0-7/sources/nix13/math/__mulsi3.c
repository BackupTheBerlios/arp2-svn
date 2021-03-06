
#if defined( __mc68000__ )

asm("
		.globl	___mulsi3

| D0 = D0 * D1

___mulsi3:	moveml	sp@(4),d0/d1
		movel	d3,sp@-
		movel	d2,sp@-
		movew	d1,d2
		mulu	d0,d2
		movel	d1,d3
		swap	d3
		mulu	d0,d3
		swap	d3
		clrw	d3
		addl	d3,d2
		swap	d0
		mulu	d1,d0
		swap	d0
		clrw	d0
		addl	d2,d0
		movel	sp@+,d2
		movel	sp@+,d3
		rts
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
# error Unsupported CPU
#endif

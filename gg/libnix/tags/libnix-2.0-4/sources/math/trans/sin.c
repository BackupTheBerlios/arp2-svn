
#if defined( __mc68000__ )

#include <proto/mathieeedoubtrans.h>

double sin(double x)
{ return IEEEDPSin(x); }

#elif defined( __i386__ )

__asm("
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include <abi.h>

ENTRY(sin)
	XMM_ONE_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_ONE
	fsin
	fnstsw	%ax
	andw	$0x400,%ax
	jnz	1f
	XMM_DOUBLE_EPILOGUE
	ret
1:	fldpi
	fadd	%st(0)
	fxch	%st(1)
2:	fprem1
	fnstsw	%ax
	andw	$0x400,%ax
	jnz	2b
	fstp	%st(1)
	fsin
	XMM_DOUBLE_EPILOGUE
	ret
");

#else
# error Unsupported CPU
#endif

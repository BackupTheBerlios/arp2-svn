
#if defined( __mc68000__ )

#include <proto/mathieeedoubtrans.h>

double asin(double x)
{ return IEEEDPAsin(x); }

#elif defined( __i386__ )

__asm("
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include <abi.h>

/* asin = atan (x / sqrt(1 - x^2)) */
ENTRY(__ieee754_asin)
	XMM_ONE_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_ONE		/* x */
	fld	%st(0)
	fmul	%st(0)			/* x^2 */
	fld1
	fsubp				/* 1 - x^2 */
	fsqrt				/* sqrt (1 - x^2) */
	fpatan
	XMM_DOUBLE_EPILOGUE
	ret

	WEAK_ALIAS(asin,__ieee754_asin)
");

#else
# error Unsupported CPU
#endif

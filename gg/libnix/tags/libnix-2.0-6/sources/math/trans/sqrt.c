
#if defined( __mc68000__ )

#include <proto/mathieeedoubtrans.h>

double sqrt(double x)
{ return IEEEDPSqrt(x); }

#elif defined( __i386__ )

__asm("
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(__ieee754_sqrt)
#ifdef __i386__
	fldl	4(%esp)
	fsqrt
#else
	sqrtsd	%xmm0,%xmm0
#endif
	ret

	WEAK_ALIAS(sqrt,__ieee754_sqrt);
");

#else
# error Unsupported CPU
#endif

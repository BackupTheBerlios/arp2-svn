
#if defined( __mc68000__ )

#include <proto/mathieeedoubtrans.h>

double log(double x)
{ return IEEEDPLog(x); }

#elif defined( __i386__ )

__asm("
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include <abi.h>

ENTRY(__ieee754_log)
	XMM_ONE_ARG_DOUBLE_PROLOGUE
	fldln2
	fldl	ARG_DOUBLE_ONE
	fyl2x
	XMM_DOUBLE_EPILOGUE
	ret

	WEAK_ALIAS(log,__ieee754_log)
");

#else
# error Unsupported CPU
#endif


#if defined( __mc68000__ )

#include <proto/mathieeedoubtrans.h>

double atan(double x)
{ return IEEEDPAtan(x); }

#elif defined( __i386__ )

__asm("
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include <abi.h>

ENTRY(atan)
	XMM_ONE_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_ONE
	fld1
	fpatan
	XMM_DOUBLE_EPILOGUE
	ret
");

#else
# error Unsupported CPU
#endif

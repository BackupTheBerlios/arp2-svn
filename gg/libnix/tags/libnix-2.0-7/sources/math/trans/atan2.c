
#if defined( __mc68000__ )

#include <proto/mathieeedoubtrans.h>

#define PI 3.14159265358979323846

static inline double atan(double x)
{ return IEEEDPAtan(x); }

double atan2(double y,double x)
{ return x>=y?(x>=-y?      atan(y/x):     -PI/2-atan(x/y)):
              (x>=-y? PI/2-atan(x/y):y>=0? PI  +atan(y/x):
                                          -PI  +atan(y/x));
}

#elif defined( __i386__ )

__asm("
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include <abi.h>

ENTRY(__ieee754_atan2)
	XMM_TWO_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_ONE
	fldl	ARG_DOUBLE_TWO
	fpatan
	XMM_DOUBLE_EPILOGUE
	ret

	WEAK_ALIAS(atan2,__ieee754_atan2)
");

#else
# error Unsupported CPU
#endif

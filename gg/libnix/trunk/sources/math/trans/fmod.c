
#if defined( __mc68000__ )

#include <proto/mathieeedoubbas.h>

static inline double floor(double x)
{ return IEEEDPFloor(x); }

static inline double ceil(double x)
{ return IEEEDPCeil(x); }

double fmod(double x,double y)
{
  double a=x/y;
  if(a>=0)
    return x-y*floor(a);
  else
    return x-y*ceil(a);
}

#elif defined( __i386__ )

__asm("
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include <abi.h>


ENTRY(__ieee754_fmod)
	XMM_TWO_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_TWO
	fldl	ARG_DOUBLE_ONE
1:	fprem
	fstsw	%ax
	btw	$10,%ax
	jc	1b
	fstp	%st(1)
	XMM_DOUBLE_EPILOGUE
	ret

	WEAK_ALIAS(fmod,__ieee754_fmod)
");
#else
# error Unsupported CPU
#endif


#if defined( __mc68000__ )

#include <proto/mathieeedoubtrans.h>

double __extendsfdf2(float x)
{ return IEEEDPFieee(x); }

#endif

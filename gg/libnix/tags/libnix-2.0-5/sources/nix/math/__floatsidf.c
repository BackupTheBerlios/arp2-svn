
#if defined( __mc68000__ )

#include <proto/mathieeedoubbas.h>

double __floatsidf(signed long x)
{ return IEEEDPFlt(x); }

#endif

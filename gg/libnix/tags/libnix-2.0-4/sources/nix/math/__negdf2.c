
#if defined( __mc68000__ )

#include <proto/mathieeedoubbas.h>

double __negdf2(double x)
{ return IEEEDPNeg(x); }

#endif

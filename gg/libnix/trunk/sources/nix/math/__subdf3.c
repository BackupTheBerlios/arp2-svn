
#if defined( __mc68000__ )

#include <proto/mathieeedoubbas.h>

double __subdf3(double x,double y)
{ return IEEEDPSub(x,y); }

#endif


#if defined( __mc68000__ )

#include <proto/mathieeedoubbas.h>

double __divdf3(double x,double y)
{ return IEEEDPDiv(x,y); }

#endif

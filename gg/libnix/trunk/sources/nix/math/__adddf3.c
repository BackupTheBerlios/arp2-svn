
#if defined( __mc68000__ )

#include <proto/mathieeedoubbas.h>

double __adddf3(double x,double y)
{ return IEEEDPAdd(x,y); }

#endif

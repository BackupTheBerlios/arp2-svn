
#if defined( __mc68000__ )

#include <proto/mathieeedoubbas.h>

double __muldf3(double x,double y)
{ return IEEEDPMul(x,y); }

#endif

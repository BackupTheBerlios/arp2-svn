
#if defined( __mc68000__ )

#include <proto/mathieeedoubtrans.h>

float __truncdfsf2(double x)
{ return IEEEDPTieee(x); }

#endif

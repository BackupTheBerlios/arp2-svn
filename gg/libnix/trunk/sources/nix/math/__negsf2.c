
#if defined( __mc68000__ )

#include <proto/mathieeesingbas.h>

float __negsf2(float x)
{ return IEEESPNeg(x); }

#endif

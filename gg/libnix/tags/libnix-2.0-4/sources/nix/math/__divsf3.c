
#if defined( __mc68000__ )

#include <proto/mathieeesingbas.h>

float __divsf3(float x,float y)
{ return IEEESPDiv(x,y); }

#endif

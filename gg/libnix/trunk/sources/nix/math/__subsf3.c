
#if defined( __mc68000__ )

#include <proto/mathieeesingbas.h>

float __subsf3(float x,float y)
{ return IEEESPSub(x,y); }

#endif


#if defined( __mc68000__ )

#include <proto/mathieeesingbas.h>

float __mulsf3(float x,float y)
{ return IEEESPMul(x,y); }

#endif


#if defined( __mc68000__ )

#include <proto/mathieeesingbas.h>

float __addsf3(float x,float y)
{ return IEEESPAdd(x,y); }

#endif

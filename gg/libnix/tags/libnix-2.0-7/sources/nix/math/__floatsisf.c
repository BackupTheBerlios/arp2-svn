
#if defined( __mc68000__ )

#include <proto/mathieeesingbas.h>

float __floatsisf(signed long x)
{ return IEEESPFlt(x); }

#endif


#if defined( __mc68000__ )

#include <proto/mathieeesingbas.h>

signed long __fixsfsi(float x)
{ return IEEESPFix(x); }

#endif

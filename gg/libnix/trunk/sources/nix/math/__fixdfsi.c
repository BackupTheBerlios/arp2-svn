
#if defined( __mc68000__ )

#include <proto/mathieeedoubbas.h>

signed long __fixdfsi(double x)
{ return IEEEDPFix(x); }

#endif

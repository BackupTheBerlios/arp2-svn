#include <math.h>

double fabs(double a)
{
	asm("fabs 1,1");
}

#include <math.h>

double acos(double x)
{
  return (fabs(atan(sqrt(1-x*x)/x)));
}

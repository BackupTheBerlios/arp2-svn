#include <math.h>

double asin(double x)
{
  return (atan(x/sqrt(1-x*x)));
}

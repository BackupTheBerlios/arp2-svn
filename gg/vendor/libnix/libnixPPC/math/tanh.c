#include <math.h>

double tanh(double x)
{
  double ex = exp(x);
  double emx = exp(-x);

  return ((ex-emx)/(ex+emx));
}

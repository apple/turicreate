#include "gsl/gsl_rng.h"
#include <math.h>

int main()
{
  // return code
  int retval = 1;

  // create a generator
  gsl_rng* generator;
  generator = gsl_rng_alloc(gsl_rng_mt19937);

  // Read a value.
  double const Result = gsl_rng_uniform(generator);

  // Check value
  double const expectedResult(0.999741748906672);
  if (fabs(expectedResult - Result) < 1.0e-6)
    retval = 0;

  // free allocated memory
  gsl_rng_free(generator);
  return retval;
}

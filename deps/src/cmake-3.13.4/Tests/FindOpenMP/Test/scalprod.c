#include <omp.h>

#ifdef __cplusplus
extern "C"
#endif
  void
  scalprod(int n, double* x, double* y, double* res)
{
  int i;
  double res_v = 0.;
#pragma omp parallel for reduction(+ : res_v)
  for (i = 0; i < n; ++i) {
    res_v += x[i] * y[i];
  }
  *res = res_v;
}

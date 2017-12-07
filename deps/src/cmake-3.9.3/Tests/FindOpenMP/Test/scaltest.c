#ifdef __cplusplus
#include <iostream>
extern "C"
#else
#include <stdio.h>
#endif
int scalprod(int n, double* x, double* y, double* res);

int main()
{
  double a[5] = { 1., 2., 3., 4., 5. };
  double b[5] = { 2., 3., 4., 5., 6. };
  double rk;
  scalprod(5, a, b, &rk);
#ifdef __cplusplus
  std::cout << rk << std::endl;
#else
  printf("%f\n", rk);
#endif
}

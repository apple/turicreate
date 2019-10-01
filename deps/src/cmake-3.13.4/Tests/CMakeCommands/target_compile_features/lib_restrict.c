
#include "lib_restrict.h"

int foo(int* restrict a, int* restrict b)
{
  (void)a;
  (void)b;
  return 0;
}

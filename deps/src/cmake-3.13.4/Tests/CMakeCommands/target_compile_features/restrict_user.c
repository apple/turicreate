
#include "lib_restrict.h"

int bar(int* restrict a, int* restrict b)
{
  (void)a;
  (void)b;
  return foo(a, b);
}

int main()
{
  return 0;
}

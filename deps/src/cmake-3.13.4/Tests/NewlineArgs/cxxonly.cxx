#include "libcxx1.h"
#include "libcxx2.h"

#include <stdio.h>

int main()
{
  if (LibCxx1Class::Method() != 2.0) {
    printf("Problem with libcxx1\n");
    return 1;
  }
#ifdef TEST_FLAG_3
  return 0;
#else
  printf("Problem with libcxx2.h include dir probably!\n");
  return 1;
#endif
}

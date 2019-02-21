#include "libc1.h"
#include <stdio.h>

int main()
{
  if (LibC1Func() != 2.0) {
    printf("Problem with libc1\n");
    return 1;
  }
  return 0;
}

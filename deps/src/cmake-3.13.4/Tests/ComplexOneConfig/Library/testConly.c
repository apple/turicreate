#include "testConly.h"
#include <stdio.h>

int CsharedFunction()
{
#ifndef TEST_C_FLAGS
  printf("TEST_C_FLAGS failed\n");
  return 0;
#else
  printf("Passed: TEST_C_FLAGS passed\n");
#endif
  return 1;
}

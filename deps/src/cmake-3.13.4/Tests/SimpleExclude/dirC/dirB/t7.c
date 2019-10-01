#include <stdio.h>

extern int tlib2func();

int tlib7func()
{
  printf("This is T7\n");

  if (tlib2func() != 2) {
    fprintf(stderr, "Something wrong with T2\n");
    return 1;
  }

  return 7;
}

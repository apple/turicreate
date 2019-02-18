#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int res = 0;
bool print = false;

void test_expression(int x, int y, const char* text)
{
  bool fail = (x) != (y);
  if (fail) {
    res++;
    printf("Problem with EXPR:");
  }
  if (fail || print) {
    printf("Expression: \"%s\" in CMake returns %d", text, (y));
    if (fail) {
      printf(" while in C returns: %d", (x));
    }
    printf("\n");
  }
}

int main(int argc, char* argv[])
{
  if (argc > 2) {
    printf("Usage: %s [print]\n", argv[0]);
    return 1;
  }

  if (argc > 1) {
    if (strcmp(argv[1], "print") != 0) {
      printf("Usage: %s [print]\n", argv[0]);
      return 1;
    }
    print = true;
  }

#include "MathTestTests.h"

  if (res != 0) {
    printf("%s: %d math tests failed\n", argv[0], res);
    return 1;
  }
  return 0;
}

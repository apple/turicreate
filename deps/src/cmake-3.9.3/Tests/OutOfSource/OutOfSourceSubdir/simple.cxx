#include <stdio.h>
#include <string.h>

#include "testdp.h"
#include "testlib.h"

extern int simple();
#ifndef NO_DEEPSRC
extern int simple2();
#endif
extern "C" int outlib();

int main()
{
  if (simple() != 123) {
    return -3;
  }
  if (strcmp(animal, "SIZZLING")) {
    fprintf(stderr, "Get definitions from a subdir did not work\n");
    return -2;
  }
  if (TestLib() != 1.0) {
    return -1;
  }
  if (outlib() != 456) {
    return -4;
  }
#ifndef NO_DEEPSRC
  if (simple2() != 789) {
    return -5;
  }
#endif
  return 0;
}

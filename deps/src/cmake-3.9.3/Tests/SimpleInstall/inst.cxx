#include "foo.h"

#ifdef STAGE_2
#include <foo/lib1.h>
#include <foo/lib2renamed.h>
#include <lib3.h>
#include <old/lib2.h>
#include <old/lib3.h>
#else
#include "lib1.h"
#include "lib2.h"
#endif

#include "lib4.h"

#include <stdio.h>

int main()
{
  if (Lib1Func() != 2.0) {
    printf("Problem with lib1\n");
    return 1;
  }
  if (Lib2Func() != 1.0) {
    printf("Problem with lib2\n");
    return 1;
  }
  if (Lib4Func() != 4.0) {
    printf("Problem with lib4\n");
    return 1;
  }
  printf("The value of Foo: %s\n", foo);
  return SomeFunctionInFoo() - 5;
}

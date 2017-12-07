
#include "depG.h"

#include "bar.h"
#include "foo.h"

#ifndef TEST_DEF
#error Expected TEST_DEF definition
#endif

int main(int, char**)
{
  DepG g;

  return g.foo();
}

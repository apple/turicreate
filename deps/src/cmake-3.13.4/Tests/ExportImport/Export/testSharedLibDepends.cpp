
#include "testSharedLibDepends.h"

int TestSharedLibDepends::foo()
{
  TestSharedLibRequired req;
  Renamed renamed;
  return req.foo() + renamed.foo();
}

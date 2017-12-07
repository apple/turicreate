
#include "testSharedLibRequiredUser.h"

#include "testSharedLibRequired.h"

int TestSharedLibRequiredUser::foo()
{
  TestSharedLibRequired req;
  return req.foo();
}

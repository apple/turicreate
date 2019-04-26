
#ifndef SHAREDLIB_DEFINE
#  error Expected SHAREDLIB_DEFINE
#endif

#ifndef SHAREDDEPENDLIB_DEFINE
#  error Expected SHAREDDEPENDLIB_DEFINE
#endif

#include "shareddependlib.h"
#include "sharedlib.h"

int main(int, char**)
{
  SharedLibObject sl;
  SharedDependLibObject sdl = sl.object();

  return sdl.foo() + sl.foo();
}

#ifndef foo_h
#include "foo.h"
#error "Precompiled header foo_precompiled.h has not been loaded."
#endif

int foo()
{
  return 0;
}

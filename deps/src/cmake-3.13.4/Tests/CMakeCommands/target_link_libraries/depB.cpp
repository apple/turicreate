
#include "depB.h"

#include "depA.h"

#include "libgenex.h"

int DepB::foo()
{
  DepA a;

  LibGenex lg;

  return a.foo() + lg.foo();
}

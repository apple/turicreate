
#include "depD.h"

int main(int, char**)
{
  DepD d;
  DepA a = d.getA();

  return d.foo() + a.foo();
}

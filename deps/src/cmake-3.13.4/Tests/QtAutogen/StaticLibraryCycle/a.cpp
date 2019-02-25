#include "a.h"
#include "b.h"

bool A::recursed = false;

A::A()
{
  if (!A::recursed) {
    A::recursed = true;
    B b;
  }
}

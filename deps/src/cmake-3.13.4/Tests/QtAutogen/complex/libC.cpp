
#include "libC.h"

LibC::LibC(QObject* parent)
  : QObject(parent)
{
}

int LibC::foo()
{
  return b.foo();
}

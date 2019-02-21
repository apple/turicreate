
#include "libB.h"

LibB::LibB(QObject* parent)
  : QObject(parent)
{
}

int LibB::foo()
{
  return a.foo();
}

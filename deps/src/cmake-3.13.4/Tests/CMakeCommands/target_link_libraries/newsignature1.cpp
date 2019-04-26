
#include "depB.h"
#include "depC.h"
#include "depIfaceOnly.h"

#include "subdirlib.h"

int main(int, char**)
{
  DepA a;
  DepB b;
  DepC c;

  DepIfaceOnly iface_only;

  SubDirLibObject sd;

  return a.foo() + b.foo() + c.foo() + iface_only.foo() + sd.foo();
}

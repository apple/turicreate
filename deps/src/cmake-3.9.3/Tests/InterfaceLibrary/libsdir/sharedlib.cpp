
#include "sharedlib.h"

SharedDependLibObject SharedLibObject::object() const
{
  SharedDependLibObject sdlo;
  return sdlo;
}
int SharedLibObject::foo() const
{
  return 0;
}

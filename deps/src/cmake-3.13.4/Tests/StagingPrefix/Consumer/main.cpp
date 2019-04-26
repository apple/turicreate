
#include "bar.h"
#include "foo.h"

int main(int, char**)
{
  Foo f;
  Bar b;
  return f.foo() + b.bar();
}

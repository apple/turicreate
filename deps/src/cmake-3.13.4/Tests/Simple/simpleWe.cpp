#include <stdio.h>

class Foo
{
public:
  Foo() { printf("This one has nonstandard extension\n"); }
  int getnum() { return 0; }
};

int bar()
{
  Foo f;
  return f.getnum();
}

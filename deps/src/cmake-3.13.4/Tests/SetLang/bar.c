#include <stdio.h>

int foo();
class A
{
public:
  A() { this->i = foo(); }
  int i;
};

int main()
{
  A a;
  if (a.i == 21) {
    printf("passed foo is 21\n");
    return 0;
  }
  printf("Failed foo is not 21\n");
  return -1;
}

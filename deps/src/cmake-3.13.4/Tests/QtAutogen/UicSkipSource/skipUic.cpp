
#include "skipUicGen.hpp"
#include "skipUicNoGen1.hpp"
#include "skipUicNoGen2.hpp"

int main(int, char**)
{
  skipGen();
  skipNoGen1();
  skipNoGen2();

  return 0;
}

// -- Function definitions
void ui_nogen1()
{
}

void ui_nogen2()
{
}

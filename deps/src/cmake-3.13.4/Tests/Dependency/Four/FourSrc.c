#include <two-test.h> /* Requires TwoCustom to be built first.  */
void NoDepAFunction();
void OneFunction();
void TwoFunction();

void FourFunction()
{
  static int count = 0;
  if (count == 0) {
    ++count;
    TwoFunction();
  }
  OneFunction();
  NoDepAFunction();
}

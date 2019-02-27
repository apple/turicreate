
#include "skipSource/qItemA.hpp"
#include "skipSource/qItemB.hpp"
#include "skipSource/qItemC.hpp"

int main(int, char**)
{
  QItemA itemA;
  QItemA itemB;
  QItemA itemC;

  // Fails to link if the symbol is not present.
  return 0;
}

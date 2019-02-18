
#include "qItemA.hpp"
#include "qItemB.hpp"
#include "qItemC.hpp"
#include "qItemD.hpp"

int main(int, char**)
{
  QItemA itemA;
  QItemB itemB;
  QItemC itemC;
  QItemD itemD;

  // Fails to link if the symbol is not present.
  return 0;
}

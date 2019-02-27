// Include code from a header that should not be compiled separately.
#include "A.hh"

#include <stdio.h>
int main()
{
  printf("#define A_VALUE %d\n", A());
  return 0;
}

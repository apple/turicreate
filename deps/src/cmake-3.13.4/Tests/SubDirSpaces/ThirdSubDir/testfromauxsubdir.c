#include <stdio.h>

void secondone();
void pair_stuff();
void pair_p_stuff();
void vcl_stuff();
#ifdef CMAKE_PAREN
void testOdd();
#endif
int main()
{
  printf("Hello from subdirectory\n");
  secondone();
#ifdef CMAKE_PAREN
  testOdd();
#endif
  pair_stuff();
  pair_p_stuff();
  vcl_stuff();
  return 0;
}

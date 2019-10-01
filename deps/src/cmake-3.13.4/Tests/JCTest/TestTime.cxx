#include <stdio.h>
#include <stdlib.h>

int main(int ac, char** av)
{
  float d = 10.0;
  for (int i = 0; i < atoi(av[1]); i++) {
    d *= .2;
  }
  printf("%f", d);
}

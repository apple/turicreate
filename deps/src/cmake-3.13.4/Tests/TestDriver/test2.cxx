#include <stdio.h>
int test2(int ac, char* av[])
{
  printf("test2\n");
  for (int i = 0; i < ac; i++)
    printf("arg %d is %s\n", ac, av[i]);
  return 0;
}

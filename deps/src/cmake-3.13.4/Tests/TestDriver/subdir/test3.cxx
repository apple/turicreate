#include <stdio.h>
int subdir_test3(int ac, char* av[])
{
  printf("test3\n");
  for (int i = 0; i < ac; i++)
    printf("arg %d is %s\n", ac, av[i]);
  return 0;
}

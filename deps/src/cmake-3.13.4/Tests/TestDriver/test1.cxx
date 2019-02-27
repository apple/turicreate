#include <stdio.h>
int testExtraStuff3();
int testExtraStuff();
int testExtraStuff2();

int test1(int ac, char* av[])
{
  if (!testExtraStuff2()) {
    return -1;
  }
  if (!testExtraStuff()) {
    return -1;
  }
  if (!testExtraStuff3()) {
    return -1;
  }

  printf("test1\n");
  for (int i = 0; i < ac; i++)
    printf("arg %d is %s\n", ac, av[i]);
  return 0;
}

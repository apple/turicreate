#include <stdio.h>
#include <string.h>
int main(int ac, char* av[])
{
  int i;
  printf("ac = [%d]\n", ac);
  for (i = 0; i < ac; i++) {
    printf("arg[%d] = %s\n", i, av[i]);
  }
  if (ac == 3) {
    if (strcmp(av[1], "arg1") == 0 && strcmp(av[2], "arg2") == 0) {
      printf("arg1 and arg2 present and accounted for!\n");
      return 0;
    }
  }
  printf("arg1 and arg2 missing!\n");
  return -1;
}

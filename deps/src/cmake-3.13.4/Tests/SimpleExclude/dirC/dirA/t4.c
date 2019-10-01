#include <stdio.h>

#ifdef __CLASSIC_C__
int main()
{
  int ac;
  char* av[];
#else
int main(int ac, char* av[])
{
#endif
  if (ac > 1000) {
    return *av[0];
  }
  printf("This is T4. This one should work.\n");
  return 0;
}

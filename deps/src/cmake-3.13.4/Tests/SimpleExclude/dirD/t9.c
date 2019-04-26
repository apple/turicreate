#include <stdio.h>

extern int tlib7func();

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
  printf("This is T9. This one should work.\n");

  if (tlib7func() != 7) {
    fprintf(stderr, "Something wrong with T7\n");
    return 1;
  }
  return 0;
}

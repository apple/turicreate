#include <stdio.h>

#ifdef __CLASSIC_C__
int main()
{
  int argc;
  char* argv[];
#else
int main(int argc, char* argv[])
{
#endif
  printf("hello assembler world, %d arguments  given\n", argc);
  return 0;
}

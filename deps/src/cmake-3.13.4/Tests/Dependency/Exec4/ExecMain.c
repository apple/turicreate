#include <stdio.h>

void FiveFunction();
void TwoFunction();

int main()
{
  FiveFunction();
  TwoFunction();

  printf("Dependency test executable ran successfully.\n");

  return 0;
}

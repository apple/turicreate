#include <stdio.h>

int main(void)
{
  int j = 0;
  if (j > 0) {
    printf("This doesn't happen.\n");
    printf("Neither does this.\n");
  }
  j = j + 1;
  if (j > 0) {
    printf("This does happen.\n");
  }

  return 0;
}

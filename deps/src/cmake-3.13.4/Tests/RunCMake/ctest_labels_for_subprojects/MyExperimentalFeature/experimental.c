#include <stdio.h>

int main(void)
{
  int i = 0;
  if (i > 0) {
    printf("This doesn't happen.\n");
    printf("Neither does this.\n");
  }
  i = i + 1;
  if (i > 0) {
    printf("This does happen.\n");
  }

  return 0;
}

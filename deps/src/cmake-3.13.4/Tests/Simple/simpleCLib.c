#include <stdio.h>

int FooBar()
{
  int class;
  int private = 10;
  for (class = 0; class < private; class ++) {
    printf("Count: %d/%d\n", class, private);
  }
  return 0;
}

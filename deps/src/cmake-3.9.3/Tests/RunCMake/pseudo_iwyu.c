#include <stdio.h>

int main(void)
{
  fprintf(stderr, "should add these lines:\n#include <...>\n");
  /* include-what-you-use always returns failure */
  return 1;
}

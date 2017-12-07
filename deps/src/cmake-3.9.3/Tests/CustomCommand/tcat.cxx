#include <stdio.h>

int main()
{
  int c;
  while ((c = getc(stdin), c != EOF)) {
    putc(c, stdout);
  }
  return 0;
}

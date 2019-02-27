#include <stdio.h>

int main()
{
  char buf[1024];
  size_t nIn = sizeof(buf);
  while (nIn == sizeof(buf)) {
    nIn = fread(buf, 1, sizeof(buf), stdin);
    if (nIn > 0) {
      size_t nOut;
      nOut = fwrite(buf, 1, nIn, stdout);
      if (nOut != nIn) {
        return 1;
      }
    }
  }
  return 0;
}

#include <string.h>

int TSD(const char* foo)
{
  if (strcmp(foo, "TEST") == 0) {
    return 0;
  }
  return 1;
}

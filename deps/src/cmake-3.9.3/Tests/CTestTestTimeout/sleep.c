#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>

int main(void)
{
  fprintf(stderr, "before sleep\n");
  fflush(stderr); /* should not be needed, but just in case */
#if defined(_WIN32)
  Sleep((TIMEOUT + 4) * 1000);
#else
  sleep((TIMEOUT + 4));
#endif
  fprintf(stderr, "after sleep\n");
  fflush(stderr); /* should not be needed, but just in case */
  return 0;
}

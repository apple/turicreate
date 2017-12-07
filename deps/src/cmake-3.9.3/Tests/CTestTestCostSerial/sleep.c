#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

/* sleeps for 1 second */
int main(int argc, char** argv)
{
#if defined(_WIN32)
  Sleep(1000);
#else
  sleep(1);
#endif
  return 0;
}

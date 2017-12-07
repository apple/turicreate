#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

/* sleeps for 5 seconds */
int main(int argc, char** argv)
{
#if defined(_WIN32)
  Sleep(5000);
#else
  sleep(5);
#endif
  return 0;
}

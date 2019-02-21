#if defined(_WIN32)
#  include <windows.h>
#else
#  include <unistd.h>
#endif

/* sleeps for 0.1 second */
int main(int argc, char** argv)
{
#if defined(_WIN32)
  Sleep(100);
#else
  usleep(100 * 1000);
#endif
  return 0;
}

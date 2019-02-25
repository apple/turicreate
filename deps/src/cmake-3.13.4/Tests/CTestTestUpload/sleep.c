#if defined(_WIN32)
#  include <windows.h>
#else
#  include <unistd.h>
#endif

/* sleeps for n seconds, where n is the argument to the program */
int main(int argc, char** argv)
{
  int time;
  if (argc > 1) {
    time = atoi(argv[1]);
  }
#if defined(_WIN32)
  Sleep(time * 1000);
#else
  sleep(time);
#endif
  return 0;
}

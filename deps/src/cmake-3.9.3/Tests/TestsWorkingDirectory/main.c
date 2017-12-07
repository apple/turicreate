#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__WATCOMC__) ||          \
                        defined(__BORLANDC__) || defined(__MINGW32__))

#include <direct.h>
#include <io.h>

#if defined(__WATCOMC__)
#include <direct.h>
#define _getcwd getcwd
#endif

static const char* Getcwd(char* buf, unsigned int len)
{
  const char* ret = _getcwd(buf, len);
  char* p = NULL;
  if (!ret) {
    fprintf(stderr, "No current working directory.\n");
    abort();
  }
  // make sure the drive letter is capital
  if (strlen(buf) > 1 && buf[1] == ':') {
    buf[0] = toupper(buf[0]);
  }
  for (p = buf; *p; ++p) {
    if (*p == '\\') {
      *p = '/';
    }
  }
  return ret;
}

#else
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

static const char* Getcwd(char* buf, unsigned int len)
{
  const char* ret = getcwd(buf, len);
  if (!ret) {
    fprintf(stderr, "No current working directory\n");
    abort();
  }
  return ret;
}

#endif

int main(int argc, char* argv[])
{
  char buf[2048];
  const char* cwd = Getcwd(buf, sizeof(buf));

  return strcmp(cwd, argv[1]);
}

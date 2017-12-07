#include <stdio.h>
#include <windows.h>

extern int lib();

struct x
{
  const char* txt;
};

int main(int argc, char** argv)
{
  int ret = 1;

  fprintf(stdout, "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)\n");

#ifdef CMAKE_RCDEFINE
  fprintf(stdout, "CMAKE_RCDEFINE defined\n");
#endif

#ifdef CMAKE_RCDEFINE_NO_QUOTED_STRINGS
  // Expect CMAKE_RCDEFINE to preprocess to exactly test.txt
  x test;
  test.txt = "*exactly* test.txt";
  fprintf(stdout, "CMAKE_RCDEFINE_NO_QUOTED_STRINGS defined\n");
  fprintf(stdout, "CMAKE_RCDEFINE is %s, and is *not* a string constant\n",
          CMAKE_RCDEFINE);
#else
  // Expect CMAKE_RCDEFINE to be a string:
  fprintf(stdout, "CMAKE_RCDEFINE='%s', and is a string constant\n",
          CMAKE_RCDEFINE);
#endif

  HRSRC hello = ::FindResource(NULL, MAKEINTRESOURCE(1025), "TEXTFILE");
  if (hello) {
    fprintf(stdout, "FindResource worked\n");
    HGLOBAL hgbl = ::LoadResource(NULL, hello);
    int datasize = (int)::SizeofResource(NULL, hello);
    if (hgbl && datasize > 0) {
      fprintf(stdout, "LoadResource worked\n");
      fprintf(stdout, "SizeofResource returned datasize='%d'\n", datasize);
      void* data = ::LockResource(hgbl);
      if (data) {
        fprintf(stdout, "LockResource worked\n");
        char* str = (char*)malloc(datasize + 4);
        if (str) {
          memcpy(str, data, datasize);
          str[datasize] = 'E';
          str[datasize + 1] = 'O';
          str[datasize + 2] = 'R';
          str[datasize + 3] = 0;
          fprintf(stdout, "str='%s'\n", str);
          free(str);

          ret = 0;

#ifdef CMAKE_RCDEFINE_NO_QUOTED_STRINGS
          fprintf(stdout, "LoadString skipped\n");
#else
          char buf[256];
          if (::LoadString(NULL, 1026, buf, sizeof(buf)) > 0) {
            fprintf(stdout, "LoadString worked\n");
            fprintf(stdout, "buf='%s'\n", buf);
          } else {
            fprintf(stdout, "LoadString failed\n");
            ret = 1;
          }
#endif
        }
      }
    }
  }

  return ret + lib();
}

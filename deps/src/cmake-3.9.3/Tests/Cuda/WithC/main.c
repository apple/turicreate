extern int use_cuda(void);

#ifdef _WIN32
#include <windows.h>
#endif

int main()
{
#ifdef _WIN32
  /* Use an API that requires CMake's "standard" C libraries.  */
  GetOpenFileName(NULL);
#endif
  return use_cuda();
}

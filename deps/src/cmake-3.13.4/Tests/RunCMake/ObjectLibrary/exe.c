#if defined(_WIN32) && defined(COMPILE_FOR_SHARED_LIB)
#  define IMPORT __declspec(dllimport)
#else
#  define IMPORT
#endif

extern IMPORT int b(void);
int main()
{
  return b();
}
#ifndef REQUIRED
#  error "REQUIRED needs to be defined"
#endif

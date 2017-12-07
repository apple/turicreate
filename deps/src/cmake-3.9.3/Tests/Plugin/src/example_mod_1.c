#include <example.h>

#include <stdio.h>

#if defined(_WIN32)
#define MODULE_EXPORT __declspec(dllexport)
#else
#define MODULE_EXPORT
#endif

#ifdef __WATCOMC__
#define MODULE_CCONV __cdecl
#else
#define MODULE_CCONV
#endif

MODULE_EXPORT int MODULE_CCONV example_mod_1_function(int n)
{
  int result = example_exe_function() + n;
  printf("world\n");
  return result;
}

#ifdef __WATCOMC__
#  define MODULE_CCONV __cdecl
#else
#  define MODULE_CCONV
#endif

int __declspec(dllimport) example_exe_function(void);
int __declspec(dllimport) example_dll_function(void);
#ifdef EXAMPLE_DLL_2
int __declspec(dllimport) example_dll_2_function(void);
#endif

__declspec(dllexport) int MODULE_CCONV example_mod_1_function(int n)
{
  return example_dll_function() +
#ifdef EXAMPLE_DLL_2
    example_dll_2_function() +
#endif
    example_exe_function() + n;
}

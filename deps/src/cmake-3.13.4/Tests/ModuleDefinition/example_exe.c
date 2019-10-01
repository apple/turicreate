extern int __declspec(dllimport) example_dll_function(void);
extern int __declspec(dllimport) example_dll_gen_function(void);
#ifdef EXAMPLE_DLL_2
extern int __declspec(dllimport) example_dll_2_function(void);
#endif
extern int __declspec(dllimport) split_dll_1(void);
extern int __declspec(dllimport) split_dll_2(void);

int example_exe_function(void)
{
  return 0;
}

int main(void)
{
  return example_dll_function() + example_dll_gen_function() +
#ifdef EXAMPLE_DLL_2
    example_dll_2_function() +
#endif
    split_dll_1() + split_dll_2() + example_exe_function();
}

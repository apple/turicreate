#ifdef _WIN32
__declspec(dllimport)
#endif
  extern void foo();
#ifdef _WIN32
__declspec(dllexport)
#endif
  void bar()
{
  foo();
}

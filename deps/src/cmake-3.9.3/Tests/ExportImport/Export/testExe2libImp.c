#if defined(_WIN32) || defined(__CYGWIN__)
#define testExe2libImp_EXPORT __declspec(dllexport)
#else
#define testExe2libImp_EXPORT
#endif

testExe2libImp_EXPORT int testExe2libImp(void)
{
  return 0;
}

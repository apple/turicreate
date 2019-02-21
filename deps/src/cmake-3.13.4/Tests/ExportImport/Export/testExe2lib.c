#if defined(_WIN32) || defined(__CYGWIN__)
#  define testExe2lib_EXPORT __declspec(dllexport)
#  define testExe2libImp_IMPORT __declspec(dllimport)
#else
#  define testExe2lib_EXPORT
#  define testExe2libImp_IMPORT
#endif

testExe2libImp_IMPORT int testExe2libImp(void);
testExe2lib_EXPORT int testExe2lib(void)
{
  return testExe2libImp();
}

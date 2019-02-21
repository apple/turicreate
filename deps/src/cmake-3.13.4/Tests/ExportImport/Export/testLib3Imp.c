#if defined(_WIN32) || defined(__CYGWIN__)
#  define testLib3Imp_EXPORT __declspec(dllexport)
#  define testLib3ImpDep_IMPORT __declspec(dllimport)
#else
#  define testLib3Imp_EXPORT
#  define testLib3ImpDep_IMPORT
#endif

testLib3ImpDep_IMPORT int testLib3ImpDep(void);
testLib3Imp_EXPORT int testLib3Imp(void)
{
  return testLib3ImpDep();
}

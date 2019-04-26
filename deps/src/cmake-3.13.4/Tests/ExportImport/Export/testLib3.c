#if defined(_WIN32) || defined(__CYGWIN__)
#  define testLib3_EXPORT __declspec(dllexport)
#  define testLib3Imp_IMPORT __declspec(dllimport)
#else
#  define testLib3_EXPORT
#  define testLib3Imp_IMPORT
#endif

testLib3Imp_IMPORT int testLib3Imp(void);
testLib3_EXPORT int testLib3(void)
{
  return testLib3Imp();
}

#if defined(_WIN32) || defined(__CYGWIN__)
#define testLib3ImpDep_EXPORT __declspec(dllexport)
#else
#define testLib3ImpDep_EXPORT
#endif

testLib3ImpDep_EXPORT int testLib3ImpDep(void)
{
  return 0;
}

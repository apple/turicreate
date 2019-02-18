#if defined(_WIN32) || defined(__CYGWIN__)
#  define testLibNoSONAME_EXPORT __declspec(dllexport)
#else
#  define testLibNoSONAME_EXPORT
#endif

testLibNoSONAME_EXPORT int testLibNoSoName(void)
{
  return 0;
}

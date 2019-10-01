#if defined(_WIN32) || defined(__CYGWIN__)
#  define testLib5_EXPORT __declspec(dllexport)
#else
#  define testLib5_EXPORT
#endif

testLib5_EXPORT int testLib5(void)
{
  return 0;
}

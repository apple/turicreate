#if defined(_WIN32) || defined(__CYGWIN__)
#define testLib4_EXPORT __declspec(dllexport)
#else
#define testLib4_EXPORT
#endif

testLib4_EXPORT int testLib4(void)
{
  return 0;
}

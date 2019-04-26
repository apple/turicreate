#if defined(_WIN32) || defined(__CYGWIN__)
#  define testExe2_EXPORT __declspec(dllexport)
#else
#  define testExe2_EXPORT
#endif

testExe2_EXPORT int testExe2Func(void)
{
  return 123;
}

int main()
{
  return 0;
}

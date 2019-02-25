#ifdef _WIN32
#  define TEST_EXPORT __declspec(dllexport)
#else
#  define TEST_EXPORT
#endif
TEST_EXPORT int testCxxModule(void)
{
  return 0;
}

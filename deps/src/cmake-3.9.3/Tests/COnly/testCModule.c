#ifdef _WIN32
#define TEST_EXPORT __declspec(dllexport)
#else
#define TEST_EXPORT
#endif
TEST_EXPORT int testCModule(void)
{
  return 0;
}

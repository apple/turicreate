#if defined(_WIN32) && defined(COMPILE_FOR_SHARED_LIB)
#  define EXPORT __declspec(dllexport)
#else
#  define EXPORT
#endif

EXPORT int a(void)
{
  return 0;
}

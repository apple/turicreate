
#ifdef _WIN32
#  define EXPORT __declspec(dllexport)
#else
#  define EXPORT
#endif

EXPORT int dynamic_base_func(int x)
{
  return x * x;
}

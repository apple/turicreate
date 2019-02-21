
#include <type_traits>

#ifdef _WIN32
#  define EXPORT __declspec(dllexport)
#else
#  define EXPORT
#endif

using tt = std::true_type;
using ft = std::false_type;
EXPORT int __host__ shared_cuda11_func(int x)
{
  return x * x + std::integral_constant<int, 17>::value;
}

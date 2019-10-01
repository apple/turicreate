
#include <type_traits>

using tt = std::true_type;
using ft = std::false_type;
int __host__ static_cuda11_func(int x)
{
  return x * x + std::integral_constant<int, 17>::value;
}

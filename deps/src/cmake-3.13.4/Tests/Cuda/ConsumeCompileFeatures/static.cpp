

#include <type_traits>

int static_cuda11_func(int);

int static_cxx11_func(int x)
{
  return static_cuda11_func(x) + std::integral_constant<int, 32>::value;
}

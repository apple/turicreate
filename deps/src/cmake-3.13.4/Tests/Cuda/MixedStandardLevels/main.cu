
#include <type_traits>

int main(int argc, char** argv)
{
  // Verify that issue #17519 Setting CXX_STANDARD breaks CUDA_STANDARD
  // selection via cxx_std_11 has been corrected
  using returnv = std::integral_constant<int, 0>;
  return returnv::value;
}

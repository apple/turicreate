
#include <iostream>

int static_cxx11_func(int);

void test_functions()
{
  auto x = static_cxx11_func(int(42));
  std::cout << x << std::endl;
}

int main(int argc, char** argv)
{
  test_functions();
  std::cout
    << "this executable doesn't use cuda code, just call methods defined"
    << std::endl;
  std::cout << "in libraries that have cuda code" << std::endl;
  return 0;
}


#include <iostream>

int static_func(int);
int file1_sq_func(int);

int test_functions()
{
  return file1_sq_func(static_func(42));
}

int main(int argc, char** argv)
{
  if (test_functions() == 1) {
    return 1;
  }
  std::cout
    << "this executable doesn't use cuda code, just call methods defined"
    << std::endl;
  std::cout << "in object files that have cuda code" << std::endl;
  return 0;
}

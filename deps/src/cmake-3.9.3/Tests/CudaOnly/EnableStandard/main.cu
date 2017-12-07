
#include <iostream>

#ifdef _WIN32
#define IMPORT __declspec(dllimport)
#else
#define IMPORT
#endif

int static_cuda11_func(int);
IMPORT int shared_cuda11_func(int);

void test_functions()
{
  static_cuda11_func(int(42));
  shared_cuda11_func(int(42));
}

int main(int argc, char** argv)
{
  test_functions();
  return 0;
}

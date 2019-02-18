
#include <iostream>

int cpp_sq_func(int);
int cu1_sq_func(int);
int cu2_sq_func(int);

bool test_functions()
{
  return (cu1_sq_func(42) == cpp_sq_func(42)) &&
    (cu2_sq_func(42) == cpp_sq_func(42));
}

int main(int argc, char** argv)
{
  int result = test_functions() ? 0 : 1;
  return result;
}

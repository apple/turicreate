#include <iostream>

int empty_1();
int subdir_empty_1();
int subdir_empty_2();

int main()
{
  int e1 = empty_1();
  int se1 = subdir_empty_1();
  int se2 = subdir_empty_2();

  std::cout << e1 << " " << se1 << " " << se2 << std::endl;

  return 0;
}

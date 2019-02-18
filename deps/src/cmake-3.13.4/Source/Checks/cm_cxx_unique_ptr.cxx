#include <memory>
int main()
{
  std::unique_ptr<int> u(new int(0));
  return *u;
}

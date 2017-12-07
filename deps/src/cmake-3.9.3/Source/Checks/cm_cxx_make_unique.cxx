#include <memory>
int main()
{
  std::unique_ptr<int> u = std::make_unique<int>(0);
  return *u;
}

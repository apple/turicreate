#include <cstdio>
#include <memory>
#include <unordered_map>

int main()
{
  std::unique_ptr<int> u(new int(0));
  return *u;
}

#include <memory>

std::auto_ptr<int> get_auto_ptr()
{
  std::auto_ptr<int> ptr;
  ptr = std::auto_ptr<int>(new int(0));
  return ptr;
}

int use_auto_ptr(std::auto_ptr<int> ptr)
{
  return *ptr;
}

int main()
{
  return use_auto_ptr(get_auto_ptr());
}

#include "test_compiler_detection_bare_features.h"

class base
{
public:
  virtual ~base() {}
  virtual void baz() = 0;
};

class foo final
{
public:
  virtual ~foo() {}
  char* bar;
  void baz() noexcept override { bar = nullptr; }
};

int main()
{
  foo f;

  return 0;
}


#define STRINGIFY_IMPL(X) #X
#define STRINGIFY(X) STRINGIFY_IMPL(X)

#include STRINGIFY(TEST)

int main()
{
  return 0;
}

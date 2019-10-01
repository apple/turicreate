
#define STRINGIFY_IMPL(X) #X
#define STRINGIFY(X) STRINGIFY_IMPL(X)

#include STRINGIFY(TEST)

int main(void)
{
  return 0;
}

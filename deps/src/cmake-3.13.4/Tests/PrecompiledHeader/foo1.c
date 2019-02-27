#ifndef foo_h
#  error "Precompiled header foo_precompiled.h has not been loaded."
#endif

int main()
{
  return foo();
}

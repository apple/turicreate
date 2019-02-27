
constexpr int foo()
{
  return 0;
}

int main(int argc, char**)
{
  typeof(argc) ret = foo();
  return ret;
}

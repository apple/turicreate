
int someFunc()
{
  auto identity = [](auto a) { return a; };
  return identity(0);
}

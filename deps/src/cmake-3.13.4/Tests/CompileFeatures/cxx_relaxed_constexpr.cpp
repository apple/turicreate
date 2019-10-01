
struct X
{
  constexpr X()
    : n(5)
  {
    n *= 2;
  }
  int n;
};

constexpr int g(const int (&is)[4])
{
  X x;
  int r = x.n;
  for (int i = 0; i < 5; ++i)
    r += i;
  for (auto& i : is)
    r += i;
  return r;
}

int someFunc()
{
  constexpr int k3 = g({ 4, 5, 6, 7 });
  return k3 - 42;
}

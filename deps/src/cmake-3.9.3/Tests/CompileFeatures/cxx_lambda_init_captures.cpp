
int someFunc()
{
  int a = 0;
  return [b = static_cast<int&&>(a)]() { return b; }
  ();
}

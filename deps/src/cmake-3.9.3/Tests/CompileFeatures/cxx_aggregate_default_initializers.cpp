
struct X
{
  int i, j, k = 42;
};

int someFunc()
{
  X a[] = { 1, 2, 3, 4, 5, 6 };
  X b[2] = { { 1, 2, 3 }, { 4, 5, 6 } };
  return a[0].k == b[0].k && a[1].k == b[1].k ? 0 : 1;
}


template <typename T>
struct A
{
  typedef T Result;
};

void someFunc()
{
  /* clang-format off */
  A<A<int>> object;
  /* clang-format on */
  (void)object;
}


template <typename T1, typename T2>
struct A
{
  typedef T1 MyT1;
  using MyT2 = T2;
};

using B = A<int, char>;
template <typename T>
using C = A<int, T>;

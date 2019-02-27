
template <typename T>
struct B
{
  B()
    : m_i(42)
  {
  }

private:
  int m_i;
  friend T;
};

struct A
{
  template <typename T>
  int getBValue(B<T> b)
  {
    return b.m_i;
  }
};

void someFunc()
{
  A a;
  B<A> b;
  a.getBValue(b);
}

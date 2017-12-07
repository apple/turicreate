
struct A
{
  int m_i;

  A(int i)
    : m_i(i)
  {
  }
};

struct B : public A
{
  using A::A;
};

void someFunc()
{
  int i = 0;
  B b(i);
}


struct A
{
  int m_i;
};

int someFunc()
{
  return sizeof(A::m_i) > 0 ? 1 : 2;
}

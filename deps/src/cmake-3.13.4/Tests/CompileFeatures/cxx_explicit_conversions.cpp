
class A
{
  int m_i;

public:
  explicit operator bool() { return m_i != 0; }
};

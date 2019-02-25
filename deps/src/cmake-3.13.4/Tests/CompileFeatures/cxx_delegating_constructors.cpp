
class Foo
{
public:
  Foo(int i);

  Foo(double d)
    : Foo(static_cast<int>(d))
  {
  }

private:
  int m_i;
};

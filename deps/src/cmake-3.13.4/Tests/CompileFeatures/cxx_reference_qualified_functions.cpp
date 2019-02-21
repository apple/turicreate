
struct test
{
  void f() & {}
  void f() && {}
};

void someFunc()
{
  test t;
  t.f();      // lvalue
  test().f(); // rvalue
}

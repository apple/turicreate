struct Foo
{
  Foo() {}
  ~Foo() {}
  Foo(Foo const&) = delete;
  Foo& operator=(Foo const&) = delete;
  int test() const { return 0; }
};

int main()
{
  Foo const foo;
  return foo.test();
}

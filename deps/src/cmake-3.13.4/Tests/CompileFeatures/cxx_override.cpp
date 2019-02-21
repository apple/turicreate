
struct A
{
  virtual void doNothing() {}
};
struct B : A
{
  void doNothing() override {}
};

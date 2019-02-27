struct A
{
};
struct B
{
  B(A) {}
};

void Func()
{
  B b{ A{} };
}

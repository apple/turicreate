int bar(int y)
{
  return y * 6;
}

int foo(int x)
{
  return x * bar(x);
}

int main()
{
  return foo(4);
}

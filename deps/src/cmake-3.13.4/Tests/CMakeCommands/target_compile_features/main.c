
int foo(int* restrict a, int* restrict b)
{
  (void)a;
  (void)b;
  return 0;
}

int main()
{
  return 0;
}

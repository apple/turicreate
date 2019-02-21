int test(int)
{
  return -1;
}

int test(int*)
{
  return 0;
}

int main()
{
  return test(nullptr);
}


double foo_ = 3.14;

double& foo()
{
  return foo_;
}

void someFunc()
{
  auto& x = foo();
}

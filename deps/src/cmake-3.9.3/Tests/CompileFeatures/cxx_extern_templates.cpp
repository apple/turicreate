
template <typename T>
void someFunc()
{
}

extern template void someFunc<int>();

void otherFunc()
{
  someFunc<int>();
}

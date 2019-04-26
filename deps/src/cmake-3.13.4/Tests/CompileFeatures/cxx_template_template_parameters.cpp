
template <template <typename> class T, typename U>
void someFunc(T<U>)
{
}

template <typename T>
struct A
{
};

void otherFunc()
{
  A<int> a;
  someFunc(a);
}

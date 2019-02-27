namespace Lib {
inline namespace Lib_1 {
template <typename T>
class A;
}

template <typename T>
void g(T);
}

struct MyClass
{
};
namespace Lib {
template <>
class A<MyClass>
{
};
}

void someFunc()
{
  Lib::A<MyClass> a;
  g(a); // ok, Lib is an associated namespace of A
}

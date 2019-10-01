
template <typename T>
class X
{
};
template <typename T>
void f(T t)
{
}
struct
{
} unnamed_obj;
void f()
{
  struct A
  {
  };
  enum
  {
    e1
  };
  typedef struct
  {
  } B;
  B b;
  X<A> x1;
  X<A*> x2;
  X<B> x3;
  f(e1);
  f(unnamed_obj);
  f(b);
  (void)x1;
  (void)x2;
  (void)x3;
}

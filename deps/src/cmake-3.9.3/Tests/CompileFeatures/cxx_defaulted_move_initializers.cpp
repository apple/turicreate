
struct A
{
  A() = default;
  A& operator=(A&&) = default;
  A(A&&) = default;
};

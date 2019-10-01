
struct A
{
  A(const A&) = delete;
  A& operator=(const A&) = delete;
};


struct point
{
  point() {}
  point(int x, int y)
    : x_(x)
    , y_(y)
  {
  }
  int x_, y_;
};
union u
{
  point p_;
  int i_;
  const char* s_;
};

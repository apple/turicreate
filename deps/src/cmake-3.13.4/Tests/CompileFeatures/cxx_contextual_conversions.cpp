
#define assert(E)                                                             \
  if (!(E))                                                                   \
    return 1;

template <class T>
class zero_init
{
public:
  zero_init()
    : val(static_cast<T>(0))
  {
  }
  zero_init(T val)
    : val(val)
  {
  }
  operator T&() { return val; }
  operator T() const { return val; }

private:
  T val;
};

int someFunc()
{
  zero_init<int*> p;
  assert(p == 0);
  p = new int(7);
  assert(*p == 7);
  delete p;

  zero_init<int> i;
  assert(i == 0);
  i = 7;
  assert(i == 7);
  switch (i) {
  }

  int* vp = new int[i];

  return 0;
}

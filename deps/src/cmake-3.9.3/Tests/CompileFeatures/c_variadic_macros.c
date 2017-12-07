
int someFunc(int i1, char c, int i2)
{
  (void)i1;
  (void)c;
  (void)i2;
  return 0;
}

#define FUNC_WRAPPER(...) someFunc(__VA_ARGS__)

void otherFunc()
{
  FUNC_WRAPPER(42, 'a', 7);
}

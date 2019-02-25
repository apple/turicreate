
int someFunc(int, char, int)
{
  return 0;
}

#define FUNC_WRAPPER(...) someFunc(__VA_ARGS__)

void otherFunc()
{
  FUNC_WRAPPER(42, 'a', 7);
}

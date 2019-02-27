void OneFunc();

void TwoFunc()
{
  static int i = 0;
  ++i;
  if (i == 1) {
    OneFunc();
  }
}

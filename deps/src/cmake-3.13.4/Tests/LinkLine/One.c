void TwoFunc();

void OneFunc()
{
  static int i = 0;
  ++i;
  if (i == 1) {
    TwoFunc();
  }
}

void OneFunction();
void FourFunction();

void ThreeFunction()
{
  static int count = 0;
  if (count == 0) {
    ++count;
    FourFunction();
  }
  OneFunction();
}

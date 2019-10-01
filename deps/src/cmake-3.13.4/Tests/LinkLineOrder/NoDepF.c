/* depends on NoDepE */
void NoDepE_func();

void NoDepF_func()
{
  static int firstcall = 1;
  if (firstcall) {
    firstcall = 0;
    NoDepE_func();
  }
}

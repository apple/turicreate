/* depends on NoDepF */
void NoDepF_func();

void NoDepE_func()
{
  static int firstcall = 1;
  if (firstcall) {
    firstcall = 0;
    NoDepF_func();
  }
}

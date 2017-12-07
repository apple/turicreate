/* depends on NoDepC and NoDepE (and hence on NoDepA, NoDepB and */
/*  NoDepF) */
void NoDepC_func();
void NoDepE_func();

void OneFunc()
{
  NoDepC_func();
  NoDepE_func();
}

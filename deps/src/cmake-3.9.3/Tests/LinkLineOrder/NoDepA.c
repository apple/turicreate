/* depends on NoDepB */
void NoDepB_func();

void NoDepA_func()
{
  NoDepB_func();
}

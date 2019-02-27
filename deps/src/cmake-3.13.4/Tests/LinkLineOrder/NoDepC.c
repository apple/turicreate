/* depends on NoDepA */
void NoDepA_func();

void NoDepC_func()
{
  NoDepA_func();
}

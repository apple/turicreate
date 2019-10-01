extern int foo1();
extern int bar2(void);
int bar1(void)
{
  return bar2();
}
int bar1_from_bar3(void)
{
  return foo1();
}

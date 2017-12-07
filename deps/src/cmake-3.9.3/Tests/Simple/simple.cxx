extern void simpleLib();
extern "C" int FooBar();
extern int bar();
extern int bar1();
int main()
{
  FooBar();
  bar();
  simpleLib();
  return 0;
}

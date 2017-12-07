extern "C" int bar(void);
void baz();

int shared()
{
  baz();
  return bar();
}

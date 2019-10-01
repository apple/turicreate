#ifndef FOO_1
#  error "foo1.h not included by /FI"
#endif
#ifndef FOO_2
#  error "foo2.h not included by /FI"
#endif
int main(void)
{
  return 0;
}

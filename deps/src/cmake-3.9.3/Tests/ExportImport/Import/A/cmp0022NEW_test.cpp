
#ifndef USING_TESTLIB2
#error Expected USING_TESTLIB2
#endif
#ifdef USING_TESTLIB3
#error Unexpected USING_TESTLIB3
#endif

int main(void)
{
  return 0;
}


#ifdef DO_GNU_TESTS

#ifdef MY_PRIVATE_DEFINE
#error Unexpected MY_PRIVATE_DEFINE
#endif

#ifndef MY_PUBLIC_DEFINE
#error Expected MY_PUBLIC_DEFINE
#endif

#ifndef MY_INTERFACE_DEFINE
#error Expected MY_INTERFACE_DEFINE
#endif

#endif

#ifdef TEST_LANG_DEFINES
#ifndef CONSUMER_LANG_CXX
#error Expected CONSUMER_LANG_CXX
#endif

#ifdef CONSUMER_LANG_C
#error Unexpected CONSUMER_LANG_C
#endif

#if !LANG_IS_CXX
#error Expected LANG_IS_CXX
#endif

#if LANG_IS_C
#error Unexpected LANG_IS_C
#endif
#endif

int main()
{
  return 0;
}

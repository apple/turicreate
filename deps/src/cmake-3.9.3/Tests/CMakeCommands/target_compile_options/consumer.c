
#ifdef TEST_LANG_DEFINES
#ifdef CONSUMER_LANG_CXX
#error Unexpected CONSUMER_LANG_CXX
#endif

#ifndef CONSUMER_LANG_C
#error Expected CONSUMER_LANG_C
#endif

#if !LANG_IS_C
#error Expected LANG_IS_C
#endif

#if LANG_IS_CXX
#error Unexpected LANG_IS_CXX
#endif
#endif

void consumer_c()
{
}

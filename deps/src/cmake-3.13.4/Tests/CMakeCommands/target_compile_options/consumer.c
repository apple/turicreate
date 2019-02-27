
// Visual Studio allows only one set of flags for C and C++.
// In a target using C++ we pick the C++ flags even for C sources.
#ifdef TEST_LANG_DEFINES_FOR_VISUAL_STUDIO
#  ifndef CONSUMER_LANG_CXX
#    error Expected CONSUMER_LANG_CXX
#  endif

#  ifdef CONSUMER_LANG_C
#    error Unexpected CONSUMER_LANG_C
#  endif

#  if !LANG_IS_CXX
#    error Expected LANG_IS_CXX
#  endif

#  if LANG_IS_C
#    error Unexpected LANG_IS_C
#  endif
#else
#  ifdef CONSUMER_LANG_CXX
#    error Unexpected CONSUMER_LANG_CXX
#  endif

#  ifndef CONSUMER_LANG_C
#    error Expected CONSUMER_LANG_C
#  endif

#  if !LANG_IS_C
#    error Expected LANG_IS_C
#  endif

#  if LANG_IS_CXX
#    error Unexpected LANG_IS_CXX
#  endif
#endif

void consumer_c()
{
}

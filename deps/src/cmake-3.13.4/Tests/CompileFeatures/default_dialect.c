
#if DEFAULT_C11
#  if __STDC_VERSION__ < 201112L
#    error Unexpected value for __STDC_VERSION__.
#  endif
#elif DEFAULT_C99
#  if __STDC_VERSION__ != 199901L
#    error Unexpected value for __STDC_VERSION__.
#  endif
#else
#  if !DEFAULT_C90
#    error Buildsystem error
#  endif
#  if defined(__STDC_VERSION__) &&                                            \
    !(defined(__SUNPRO_C) && __STDC_VERSION__ == 199409L)
#    error Unexpected __STDC_VERSION__ definition
#  endif
#endif

int main()
{
  return 0;
}

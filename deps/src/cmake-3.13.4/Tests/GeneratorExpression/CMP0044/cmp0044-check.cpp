
#ifdef Type_Is_
#  if !Result
#    error Result should be 1 in WARN mode
#  endif
#endif

#ifdef Type_Is_NEW
#  if Result
#    error Result should be 0 in NEW mode
#  endif
#endif

#ifdef Type_Is_OLD
#  if !Result
#    error Result should be 1 in OLD mode
#  endif
#endif

#if !defined(Type_Is_) && !defined(Type_Is_OLD) && !defined(Type_Is_NEW)
#  error No expected definition present
#endif

void foo(void)
{
}

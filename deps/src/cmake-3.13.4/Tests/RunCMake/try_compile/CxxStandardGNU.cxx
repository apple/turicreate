#if __cplusplus != 201103L &&                                                 \
  !(__cplusplus < 201103L && defined(__GXX_EXPERIMENTAL_CXX0X__))
#  error "Not GNU C++ 11 mode!"
#endif
#ifndef __STRICT_ANSI__
#  error "Not GNU C++ strict ANSI!"
#endif
int main()
{
  return 0;
}

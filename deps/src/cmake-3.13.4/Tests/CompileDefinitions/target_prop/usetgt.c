#ifndef TGT_DEF
#  error TGT_DEF incorrectly not defined
#endif
#ifndef TGT_TYPE_STATIC_LIBRARY
#  error TGT_TYPE_STATIC_LIBRARY incorrectly not defined
#endif
#ifdef TGT_TYPE_EXECUTABLE
#  error TGT_TYPE_EXECUTABLE incorrectly defined
#endif
int main(void)
{
  return 0;
}

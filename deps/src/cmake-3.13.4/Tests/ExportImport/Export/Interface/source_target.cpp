
#ifndef USE_FROM_BUILD_DIR
#  error Expected define USE_FROM_BUILD_DIR
#endif

#ifdef USE_FROM_INSTALL_DIR
#  error Unexpected define USE_FROM_INSTALL_DIR
#endif

int source_symbol()
{
  return 42;
}

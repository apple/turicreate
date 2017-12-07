
#ifdef USE_FROM_BUILD_DIR
#error Unexpected define USE_FROM_BUILD_DIR
#endif

#ifndef USE_FROM_INSTALL_DIR
#error Expected define USE_FROM_INSTALL_DIR
#endif

int source_symbol()
{
  return 42;
}

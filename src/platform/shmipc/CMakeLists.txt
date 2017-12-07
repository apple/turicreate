project(shmipc)

make_library(shmipc
  SOURCES
    shmipc.cpp
    shmipc_garbage_collect.cpp
  REQUIRES
    logger fileio
  )

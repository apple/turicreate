project(shmipc)

make_library(shmipc OBJECT
  SOURCES
    shmipc.cpp
    shmipc_garbage_collect.cpp
  REQUIRES
    logger fileio
)

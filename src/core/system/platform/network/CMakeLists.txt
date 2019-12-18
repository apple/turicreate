project(Turi)

make_library(network OBJECT
  SOURCES
    net_util.cpp
  REQUIRES
    logger
)

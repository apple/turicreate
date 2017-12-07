project(startup_teardown)

make_library(startup_teardown
  SOURCES
    startup_teardown.cpp
  REQUIRES
    logger fileio parallel timer random crash_handler pylambda
  )

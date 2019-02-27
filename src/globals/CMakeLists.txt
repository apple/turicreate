project(globals)

make_library(globals OBJECT
  SOURCES
    globals.cpp
  REQUIRES
    logger
    flexible_type
    process
  EXTERNAL_VISIBILITY
)

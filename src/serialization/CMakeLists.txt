project(serialization)

make_library(serialization OBJECT
  SOURCES
    dir_archive.cpp
  REQUIRES
    logger
    fileio
    util
    random
  EXTERNAL_VISIBILITY
)

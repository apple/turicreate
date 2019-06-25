project(util)

make_library(util OBJECT
  SOURCES
    any.cpp
    cityhash_tc.cpp
    fs_util.cpp
    md5.cpp
    safe_circular_char_buffer.cpp
    testing_utils.cpp
    web_util.cpp
    string_util.cpp
    syserr_reporting.cpp
  REQUIRES
    logger
    flexible_type
    boost
  EXTERNAL_VISIBILITY
)

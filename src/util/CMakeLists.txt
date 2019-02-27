project(util)

make_library(util OBJECT
  SOURCES
    any.cpp
    basic_types.cpp
    cityhash_tc.cpp
    fs_util.cpp
    md5.cpp
    safe_circular_char_buffer.cpp
    string_util.cpp
    syserr_reporting.cpp
    testing_utils.cpp
    web_util.cpp
  REQUIRES
    logger
    flexible_type
    boost
  EXTERNAL_VISIBILITY
)

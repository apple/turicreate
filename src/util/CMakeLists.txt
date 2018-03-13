project(util)

make_library(util 
SOURCES 
  any.cpp 
  cityhash_tc.cpp 
  syserr_reporting.cpp 
  md5.cpp 
  testing_utils.cpp 
  fs_util.cpp
  safe_circular_char_buffer.cpp
  web_util.cpp
REQUIRES 
  logger 
  flexible_type
  boost
EXTERNAL_VISIBILITY )


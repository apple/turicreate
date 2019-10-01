
cmake_policy(SET CMP0043 NEW)

add_library(foo empty.cpp)
set_property(TARGET foo
  PROPERTY COMPILE_DEFINITIONS_DEBUG "DEBUG_MODE"
)

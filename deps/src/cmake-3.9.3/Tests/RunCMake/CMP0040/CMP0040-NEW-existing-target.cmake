cmake_policy(SET CMP0040 NEW)

add_library(foobar empty.cpp)

add_custom_command(TARGET foobar PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo hello world
)

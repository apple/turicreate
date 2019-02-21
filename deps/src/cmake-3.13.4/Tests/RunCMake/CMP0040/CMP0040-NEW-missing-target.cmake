cmake_policy(SET CMP0040 NEW)

add_custom_command(TARGET foobar PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E hello world
)


add_library(empty empty.cpp)
add_custom_command(
  TARGET empty
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/input.h.in
  COMMAND ${CMAKE_COMMAND}
  ARGS -E copy ${CMAKE_CURRENT_SOURCE_DIR}/input.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/input.h
  OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/input.h
  DEPENDS ${CMAKE_COMMAND}
)

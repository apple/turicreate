
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

add_library(headeriface INTERFACE)

add_custom_target(headeriface_gen
  COMMENT "Generating iface_header_builddir.h"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/iface_header_builddir.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/iface_header_builddir.h
  VERBATIM
  )
add_dependencies(headeriface headeriface_gen)

cmake_minimum_required (VERSION 2.8)
project(LinkInterfaceLoop C)

# Add a shared library that incorrectly names itself as a
# dependency, thus forming a cycle.
add_library(A SHARED IMPORTED)
set_target_properties(A PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES A
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/dirA/A"
  )

# Add a shared library that incorrectly names itself in
# its link interface, thus forming a cycle.
add_library(B SHARED IMPORTED)
set_target_properties(B PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES B
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/dirB/B"
  )

# Add a shared library with an empty link interface
# that depends on two shared libraries.
add_library(C SHARED lib.c)
set_property(TARGET C PROPERTY LINK_INTERFACE_LIBRARIES "")
target_link_libraries(C B A)

add_executable(main main.c)
target_link_libraries(main C)

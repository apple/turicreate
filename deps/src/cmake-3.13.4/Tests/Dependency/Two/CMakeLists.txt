include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library( Two TwoSrc.c )
target_link_libraries( Two Three )

# Setup a target to cause failure if Two does not depend on it or if
# Two actually links to it.  This will test that a utility dependency
# on a library target works properly.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/two-test.h
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_CURRENT_SOURCE_DIR}/two-test.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/two-test.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/two-test.h.in
  )
add_library( TwoCustom TwoCustomSrc.c ${CMAKE_CURRENT_BINARY_DIR}/two-test.h)
set_target_properties(TwoCustom PROPERTIES EXCLUDE_FROM_ALL 1)
target_link_libraries(TwoCustom Three)

# Add a utility dependency to make sure it works without linking.
add_dependencies(Two TwoCustom)

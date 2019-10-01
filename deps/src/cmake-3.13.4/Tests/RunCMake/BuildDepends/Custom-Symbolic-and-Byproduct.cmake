add_custom_command(
  OUTPUT gen-byproduct gen-byproduct-stamp
  BYPRODUCTS byproduct
  COMMAND ${CMAKE_COMMAND} -E touch gen-byproduct-stamp
  COMMAND ${CMAKE_COMMAND} -E copy_if_different gen-byproduct-stamp byproduct
  )
set_property(SOURCE gen-byproduct PROPERTY SYMBOLIC 1)
add_custom_target(produce DEPENDS gen-byproduct)

add_custom_command(
  OUTPUT use-byproduct
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/byproduct
  COMMAND ${CMAKE_COMMAND} -E sleep 1.125 # workaround buggy filesystem timestamps
  COMMAND ${CMAKE_COMMAND} -E touch use-byproduct
  )
add_custom_target(drive ALL DEPENDS use-byproduct)
add_dependencies(drive produce)

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/check-$<LOWER_CASE:$<CONFIG>>.cmake CONTENT "
if (check_step EQUAL 1)
  set(check_pairs
    \"${CMAKE_CURRENT_BINARY_DIR}/use-byproduct|${CMAKE_CURRENT_BINARY_DIR}/gen-byproduct-stamp\"
    )
else()
  set(check_pairs
    \"${CMAKE_CURRENT_BINARY_DIR}/gen-byproduct-stamp|${CMAKE_CURRENT_BINARY_DIR}/use-byproduct\"
    )
endif()
")

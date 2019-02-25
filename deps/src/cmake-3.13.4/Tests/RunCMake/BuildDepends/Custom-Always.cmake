add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/before-always
  COMMAND ${CMAKE_COMMAND} -E touch before-always
  )
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/always
  COMMAND ${CMAKE_COMMAND} -E touch always-updated
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/before-always
  )
set_property(SOURCE always PROPERTY SYMBOLIC 1)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/after-always
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/always
  COMMAND ${CMAKE_COMMAND} -E touch after-always
  )

add_custom_target(drive ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/after-always)

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/check-$<LOWER_CASE:$<CONFIG>>.cmake CONTENT "
set(check_pairs
  \"${CMAKE_CURRENT_BINARY_DIR}/always-updated|${CMAKE_CURRENT_BINARY_DIR}/before-always\"
  \"${CMAKE_CURRENT_BINARY_DIR}/after-always|${CMAKE_CURRENT_BINARY_DIR}/always-updated\"
  )
")

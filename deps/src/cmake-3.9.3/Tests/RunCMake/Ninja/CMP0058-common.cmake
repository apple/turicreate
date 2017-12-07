add_custom_command(
  OUTPUT output1
  ${byproducts}
  COMMAND ${CMAKE_COMMAND} -E touch output1
  COMMAND ${CMAKE_COMMAND} -E touch byproduct1a
  COMMAND ${CMAKE_COMMAND} -E touch byproduct1b
  )
add_custom_target(Drive1 ALL DEPENDS output1)
add_custom_command(
  OUTPUT output2
  COMMAND ${CMAKE_COMMAND} -E copy output1 output2
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/output1
          ${CMAKE_CURRENT_BINARY_DIR}/byproduct1a
          ${CMAKE_CURRENT_BINARY_DIR}/byproduct1b
  )
add_custom_target(Drive2 ALL DEPENDS output2)
add_dependencies(Drive2 Drive1)

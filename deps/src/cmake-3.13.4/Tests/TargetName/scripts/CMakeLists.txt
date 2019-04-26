if(NOT CMAKE_BINARY_DIR STREQUAL "${CMAKE_SOURCE_DIR}")
  add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hello_world
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_SOURCE_DIR}/hello_world ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hello_world
  )
  add_custom_target(
  hello_world_copy ALL
  DEPENDS #hello_world
  ${CMAKE_CURRENT_BINARY_DIR}/hello_world
  )
endif()


cmake_policy(SET CMP0021 OLD)
add_executable(cmp0021exe main.cpp)

if(NOT CMAKE_CURRENT_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/includes")
  execute_process(COMMAND ${CMAKE_COMMAND} -E
    copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/includes"
    "${CMAKE_CURRENT_BINARY_DIR}/includes"
  )
endif()
set_property(TARGET cmp0021exe PROPERTY
  INCLUDE_DIRECTORIES includes/cmp0021)

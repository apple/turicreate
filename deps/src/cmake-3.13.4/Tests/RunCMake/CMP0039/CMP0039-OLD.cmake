
cmake_policy(SET CMP0039 OLD)

add_custom_target(utility
  COMMAND ${CMAKE_COMMAND} -E echo test
)
target_link_libraries(utility m)

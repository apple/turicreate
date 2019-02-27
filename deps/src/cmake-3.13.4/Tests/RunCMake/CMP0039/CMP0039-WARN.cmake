
add_custom_target(utility
  COMMAND ${CMAKE_COMMAND} -E echo test
)
target_link_libraries(utility m)

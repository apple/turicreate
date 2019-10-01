
add_custom_target(check ALL
  COMMAND ${CMAKE_COMMAND} -E echo check
)

target_include_directories(check PRIVATE somedir)


enable_language(C)

add_custom_target(empty
  COMMAND ${CMAKE_COMMAND} -E echo $<COMPILE_LANGUAGE>
)

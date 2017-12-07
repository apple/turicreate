
enable_language(C)

add_library(empty empty.c)

add_custom_command(TARGET empty PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo $<COMPILE_LANGUAGE>
)

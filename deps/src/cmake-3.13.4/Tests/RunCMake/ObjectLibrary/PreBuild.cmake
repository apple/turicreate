add_library(A OBJECT a.c)
add_custom_command(TARGET A PRE_BUILD
  COMMAND  ${CMAKE_COMMAND} -E echo "A pre-build"
  )

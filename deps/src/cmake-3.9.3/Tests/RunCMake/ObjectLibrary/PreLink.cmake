add_library(A OBJECT a.c)
add_custom_command(TARGET A PRE_LINK
  COMMAND  ${CMAKE_COMMAND} -E echo "A pre-link"
  )

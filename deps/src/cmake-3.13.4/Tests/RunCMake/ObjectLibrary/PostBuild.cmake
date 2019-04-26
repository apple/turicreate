add_library(A OBJECT a.c)
add_custom_command(TARGET A POST_BUILD
  COMMAND  ${CMAKE_COMMAND} -E echo "A post-build"
  )

add_custom_command(
  OUTPUT output.txt
  COMMAND ${CMAKE_COMMAND} -E echo CustomCommand > output.txt
  )
add_custom_target(CustomTarget ALL DEPENDS output.txt)

cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
add_custom_command(
  OUTPUT output.txt
  COMMAND ${CMAKE_COMMAND} -E echo CustomCommand > output.txt
  )
add_custom_target(CustomTarget ALL DEPENDS output.txt)
add_custom_target(CustomTarget2 ALL DEPENDS output.txt)
add_custom_target(CustomTarget3 ALL DEPENDS output.txt)

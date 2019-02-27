cmake_minimum_required(VERSION 3.0)
project(BuildDependsExternal NONE)
if(NOT DEFINED external_in)
  message(FATAL_ERROR "Define external_in")
endif()
if(NOT DEFINED external_out)
  message(FATAL_ERROR "Define external_out")
endif()
add_custom_command(
  OUTPUT ${external_out}
  COMMAND ${CMAKE_COMMAND} -E copy ${external_in} ${external_out}
  DEPENDS ${external_in}
  )
add_custom_target(drive ALL DEPENDS ${external_out})

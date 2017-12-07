cmake_minimum_required(VERSION 2.8.12)

project(launcher_custom_command_test_project)

include(CTest)

add_custom_command(
  OUTPUT test1.txt
  COMMAND ${CMAKE_COMMAND}
  ARGS -DTESTID=1 -P "${CMAKE_CURRENT_SOURCE_DIR}/command.cmake"
)

add_custom_command(
  OUTPUT test2.txt
  COMMAND ${CMAKE_COMMAND}
  ARGS -DTESTID=2 -P "${CMAKE_CURRENT_SOURCE_DIR}/command.cmake"
)

add_custom_target(mytarget ALL DEPENDS test1.txt test2.txt)

cmake_minimum_required(VERSION 3.10)
project(CommandLength C)

add_executable(CommandLength test.c)
add_custom_command(TARGET CommandLength POST_BUILD VERBATIM
  COMMAND ${CMAKE_COMMAND} -E make_directory log)

set(msg "xxxx $$$$ yyyy")
set(msg "${msg} ${msg}")
set(msg "${msg} ${msg}")
set(msg "${msg} ${msg}")
set(msg "${msg} ${msg}")
foreach(i RANGE 1 1000)
  add_custom_command(TARGET CommandLength POST_BUILD VERBATIM
    COMMAND ${CMAKE_COMMAND} -E echo "${i} ${msg}" > log/${i}
    )
endforeach()

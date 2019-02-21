cmake_minimum_required (VERSION 2.6)
project(SubProject)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/gen.cxx) # require generation
add_custom_command(
  OUTPUT gen.cxx
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gen.cxx.in
  COMMAND ${CMAKE_COMMAND} -E copy
             ${CMAKE_CURRENT_SOURCE_DIR}/gen.cxx.in gen.cxx
  )
add_custom_target(gen DEPENDS gen.cxx)
add_library(bar bar.cxx)
target_include_directories(bar PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(bar gen)
add_executable(car car.cxx)
add_subdirectory(foo)

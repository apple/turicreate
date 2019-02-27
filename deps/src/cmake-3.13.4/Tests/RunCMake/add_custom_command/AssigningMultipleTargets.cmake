enable_language(CXX)

add_custom_command(OUTPUT generated.cpp
  MAIN_DEPENDENCY a.c
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/generate-once.cmake ${CMAKE_CURRENT_BINARY_DIR}/generated.cpp
  VERBATIM)

add_executable(exe1 ${CMAKE_CURRENT_BINARY_DIR}/generated.cpp)
add_executable(exe2 ${CMAKE_CURRENT_BINARY_DIR}/generated.cpp)
add_executable(exe3 ${CMAKE_CURRENT_BINARY_DIR}/generated.cpp)

add_dependencies(exe1 exe2)
add_dependencies(exe3 exe1)

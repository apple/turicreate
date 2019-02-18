cmake_minimum_required(VERSION 3.9)

enable_language(C)

set(CMAKE_BUILD_WITH_INSTALL_RPATH 1)
add_library(mylib SHARED obj1.c)
add_executable(myexe testobj1.c)
target_link_libraries(myexe mylib)
set_property(TARGET myexe PROPERTY INSTALL_RPATH "${CMAKE_CURRENT_BINARY_DIR}/root-all/bin")
set_target_properties(mylib PROPERTIES VERSION 1.0 SOVERSION 1)

install(TARGETS mylib myexe
  DESTINATION bin
  )

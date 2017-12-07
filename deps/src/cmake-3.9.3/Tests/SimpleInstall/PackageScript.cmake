message("This is packaging script")
message("It writes a file with all variables available in ${CMAKE_INSTALL_PREFIX}/AllVariables.txt")

file(WRITE ${CMAKE_INSTALL_PREFIX}/AllVariables.txt "")
get_cmake_property(res VARIABLES)
foreach(var ${res})
  file(APPEND ${CMAKE_INSTALL_PREFIX}/AllVariables.txt
             "${var} \"${${var}}\"\n")
endforeach()


cmake_minimum_required (VERSION 3.0)
project(SystemInformation)

include_directories("This does not exists")
get_directory_property(incl INCLUDE_DIRECTORIES)
set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "${SystemInformation_BINARY_DIR};${SystemInformation_SOURCE_DIR}")

message("To prevent CTest from stripping output, you have to display: CTEST_FULL_OUTPUT")


configure_file(${SystemInformation_SOURCE_DIR}/SystemInformation.in
${SystemInformation_BINARY_DIR}/SystemInformation.out)
configure_file(${SystemInformation_SOURCE_DIR}/DumpInformation.h.in
${SystemInformation_BINARY_DIR}/DumpInformation.h)
add_executable(SystemInformation DumpInformation.cxx)

macro(FOO args)
  message("Test macro")
endmacro()

FOO(lala)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/AllVariables.txt "")
get_cmake_property(res VARIABLES)
foreach(var ${res})
  file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/AllVariables.txt
             "${var} \"${${var}}\"\n")
endforeach()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/AllCommands.txt "")
get_cmake_property(res COMMANDS)
foreach(var ${res})
  file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/AllCommands.txt
             "${var}\n")
endforeach()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/AllMacros.txt "")
get_cmake_property(res MACROS)
foreach(var ${res})
  file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/AllMacros.txt
             "${var}\n")
endforeach()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/OtherProperties.txt "")
get_directory_property(res INCLUDE_DIRECTORIES)
foreach(var ${res})
  file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/OtherProperties.txt
    "INCLUDE_DIRECTORY: ${var}\n")
endforeach()

get_directory_property(res LINK_DIRECTORIES)
foreach(var ${res})
  file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/OtherProperties.txt
    "LINK_DIRECTORIES: ${var}\n")
endforeach()

get_directory_property(res INCLUDE_REGULAR_EXPRESSION)
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/OtherProperties.txt
  "INCLUDE_REGULAR_EXPRESSION: ${res}\n")

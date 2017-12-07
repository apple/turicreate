cmake_minimum_required(VERSION 3.7.0)
project(CompilerIdCSharp CSharp)

foreach(v
    CMAKE_CSharp_COMPILER
    CMAKE_CSharp_COMPILER_ID
    CMAKE_CSharp_COMPILER_VERSION
    )
  if(${v})
    message(STATUS "${v}=[${${v}}]")
  else()
    message(SEND_ERROR "${v} not set!")
  endif()
endforeach()

# Version numbers may only contain numbers and periods.
if(NOT CMAKE_CSharp_COMPILER_VERSION MATCHES
    "^([0-9]+)(\\.([0-9]+))?(\\.([0-9]+))?(\\.([0-9]+))?$"
    )
  message(SEND_ERROR "Compiler version is not numeric!")
endif()

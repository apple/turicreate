cmake_minimum_required(VERSION 2.8.9)
project(CompilerIdC C)

foreach(v
    CMAKE_C_COMPILER
    CMAKE_C_COMPILER_ID
    CMAKE_C_COMPILER_VERSION
    )
  if(${v})
    message(STATUS "${v}=[${${v}}]")
  else()
    message(SEND_ERROR "${v} not set!")
  endif()
endforeach()

# Version numbers may only contain numbers and periods.
if(NOT CMAKE_C_COMPILER_VERSION MATCHES
    "^([0-9]+)(\\.([0-9]+))?(\\.([0-9]+))?(\\.([0-9]+))?$"
    )
  message(SEND_ERROR "Compiler version is not numeric!")
endif()

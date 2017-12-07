cmake_policy(SET CMP0064 NEW)

if(NOT TEST TestThatDoesNotExist)
  message(STATUS "if NOT TestThatDoesNotExist is true")
endif()

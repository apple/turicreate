cmake_policy(SET CMP0064 OLD)

if(TEST)
  message(FATAL_ERROR "TEST was not recognized to be undefined")
else()
  message(STATUS "TEST was treated as a variable")
endif()

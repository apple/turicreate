

if(TEST)
  message(FATAL_ERROR "TEST was not recognized to be undefined")
else()
  message(STATUS "TEST was treated as a variable")
endif()

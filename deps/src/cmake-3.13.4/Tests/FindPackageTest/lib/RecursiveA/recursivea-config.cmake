# Test config file.
if(NOT "${RecursiveA_FIND_COMPONENTS}" STREQUAL "A")
  message(FATAL_ERROR "find_package(RecursiveA NO_MODULE) did not forward components")
endif()

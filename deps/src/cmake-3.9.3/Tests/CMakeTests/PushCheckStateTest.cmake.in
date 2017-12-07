include(CMakePushCheckState)

set(CMAKE_EXTRA_INCLUDE_FILES file1)
set(CMAKE_REQUIRED_INCLUDES dir1)
set(CMAKE_REQUIRED_DEFINITIONS defs1 )
set(CMAKE_REQUIRED_LIBRARIES lib1)
set(CMAKE_REQUIRED_FLAGS flag1)
set(CMAKE_REQUIRED_QUIET 1)

cmake_push_check_state()

set(CMAKE_EXTRA_INCLUDE_FILES file2)
set(CMAKE_REQUIRED_INCLUDES dir2)
set(CMAKE_REQUIRED_DEFINITIONS defs2)
set(CMAKE_REQUIRED_LIBRARIES lib2)
set(CMAKE_REQUIRED_FLAGS flag2)
set(CMAKE_REQUIRED_QUIET 2)

cmake_push_check_state()

set(CMAKE_EXTRA_INCLUDE_FILES file3)
set(CMAKE_REQUIRED_DEFINITIONS defs3)
set(CMAKE_REQUIRED_INCLUDES dir3)
set(CMAKE_REQUIRED_DEFINITIONS defs3)
set(CMAKE_REQUIRED_LIBRARIES lib3)
set(CMAKE_REQUIRED_FLAGS flag3)
set(CMAKE_REQUIRED_QUIET 3)

cmake_pop_check_state()

foreach(pair IN ITEMS
  EXTRA_INCLUDE_FILES|file2
  REQUIRED_INCLUDES|dir2
  REQUIRED_DEFINITIONS|defs2
  REQUIRED_LIBRARIES|lib2
  REQUIRED_FLAGS|flag2
  REQUIRED_QUIET|2
  )
  string(REPLACE "|" ";" pair "${pair}")
  list(GET pair 0 var)
  list(GET pair 1 expected)
  if (NOT "${CMAKE_${var}}" STREQUAL "${expected}")
    set(fatal TRUE)
    message("ERROR: CMAKE_${var} is \"${CMAKE_${var}}\" (expected \"${expected}\")" )
  endif()
endforeach()

cmake_pop_check_state()

foreach(pair IN ITEMS
  EXTRA_INCLUDE_FILES|file1
  REQUIRED_INCLUDES|dir1
  REQUIRED_DEFINITIONS|defs1
  REQUIRED_LIBRARIES|lib1
  REQUIRED_FLAGS|flag1
  REQUIRED_QUIET|1
  )
  string(REPLACE "|" ";" pair "${pair}")
  list(GET pair 0 var)
  list(GET pair 1 expected)
  if (NOT "${CMAKE_${var}}" STREQUAL "${expected}")
    set(fatal TRUE)
    message("ERROR: CMAKE_${var} is \"${CMAKE_${var}}\" (expected \"${expected}\")" )
  endif()
endforeach()

if(fatal)
  message(FATAL_ERROR "cmake_push_check_state() test failed")
endif()

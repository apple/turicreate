# This test will verify if CheckSymbolExists only report symbols available
# for linking that really are. You can find some documentation on this in
# bug 11333 where we found out that gcc would optimize out the actual
# reference to the symbol, so symbols that are in fact _not_ available in the
# given libraries (but seen in header) were reported as present.
#
# If you change this test do not forget to change the CheckCXXSymbolExists
# test, too.

project(CheckSymbolExists C)

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(CMAKE_REQUIRED_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}")

include(CheckSymbolExists)

foreach(_config_type Release RelWithDebInfo MinSizeRel Debug)
  set(CMAKE_TRY_COMPILE_CONFIGURATION ${_config_type})
  unset(CSE_RESULT_${_config_type} CACHE)
  message(STATUS "Testing configuration ${_config_type}")

  check_symbol_exists(non_existent_function_for_symbol_test "cm_cse.h" CSE_RESULT_${_config_type})

  if (CSE_RESULT_${_config_type})
    message(SEND_ERROR "CheckSymbolExists reported a nonexistent symbol as existing in configuration ${_config_type}")
  endif ()
endforeach()

set(CMAKE_TRY_COMPILE_CONFIGURATION ${CMAKE_BUILD_TYPE})
unset(CSE_RESULT_ERRNO CACHE)

check_symbol_exists(errno "errno.h" CSE_RESULT_ERRNO)

if (NOT CSE_RESULT_ERRNO)
  message(SEND_ERROR "CheckSymbolExists did not find errno in <errno.h>")
else ()
  message(STATUS "errno found as expected")
endif ()

if (CMAKE_COMPILER_IS_GNUCC)
  string(APPEND CMAKE_C_FLAGS " -O3")
  unset(CSE_RESULT_O3 CACHE)
  message(STATUS "Testing with optimization -O3")

  check_symbol_exists(non_existent_function_for_symbol_test "cm_cse.h" CSE_RESULT_O3)

  if (CSE_RESULT_O3)
    message(SEND_ERROR "CheckSymbolExists reported a nonexistent symbol as existing with optimization -O3")
  endif ()
endif ()

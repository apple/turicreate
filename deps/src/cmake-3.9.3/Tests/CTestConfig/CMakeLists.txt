cmake_minimum_required(VERSION 2.8)
project(CTestConfig)

include(CTest)


# We expect this configure to occur through a 'ctest -D Experimental' or a
# 'ctest -S script.cmake' call.
#
# In either case, we expect CMAKE_BUILD_TYPE to be defined for single-configuration
# build trees and not defined for multi-configuration build trees.
#
if(CMAKE_CONFIGURATION_TYPES)
  # multi-configuration: expect not defined, error if defined
  if(DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "")
    message(FATAL_ERROR "CMAKE_CONFIGURATION_TYPES='${CMAKE_CONFIGURATION_TYPES}' CMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}' is defined and non-empty (but should not be for a multi-configuration generator)")
  endif()
else()
  # single-configuration: expect defined, error if not defined
  if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE is not defined or is empty (but should be defined and non-empty for a single-configuration generator)")
  endif()
endif()


if(DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "")
  add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
endif()

add_executable(ctc CTestConfig.cxx)


foreach(cfg ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
  add_test(NAME ctc-${cfg} CONFIGURATIONS ${cfg} COMMAND ctc --config $<CONFIGURATION>)

  if(CMAKE_CONFIGURATION_TYPES)
    set_property(TEST ctc-${cfg}
      PROPERTY PASS_REGULAR_EXPRESSION "CMAKE_INTDIR is ${cfg}")
    set_property(TEST ctc-${cfg}
      PROPERTY FAIL_REGULAR_EXPRESSION "CMAKE_BUILD_TYPE is")
  else()
    set_property(TEST ctc-${cfg}
      PROPERTY PASS_REGULAR_EXPRESSION "CMAKE_BUILD_TYPE is ${cfg}")
    set_property(TEST ctc-${cfg}
      PROPERTY FAIL_REGULAR_EXPRESSION "CMAKE_INTDIR is")
  endif()
endforeach()

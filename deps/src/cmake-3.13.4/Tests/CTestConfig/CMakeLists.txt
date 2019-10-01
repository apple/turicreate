cmake_minimum_required(VERSION 3.9)
project(CTestConfig)

include(CTest)


# We expect this configure to occur through a 'ctest -D Experimental' or a
# 'ctest -S script.cmake' call.
#
# In either case, we expect CMAKE_BUILD_TYPE to be defined for single-configuration
# build trees and not defined for multi-configuration build trees. The value of
# CMAKE_CONFIGURATION_TYPES should not be relied upon to determine whether we
# are using a multi-config generator or not, the GENERATOR_IS_MULTI_CONFIG
# global property is the canonical way to do that as of CMake 3.9.
#
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_isMultiConfig)
  if(NOT DEFINED CMAKE_CONFIGURATION_TYPES OR CMAKE_CONFIGURATION_TYPES STREQUAL "")
    message(FATAL_ERROR "CMAKE_CONFIGURATION_TYPES is not defined or is empty "
        "(but must be defined and non-empty for a multi-configuration generator)")
  endif()
  if(DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}' is defined and non-empty "
        "(but should not be for a multi-configuration generator)")
  endif()
  set(_configs ${CMAKE_CONFIGURATION_TYPES})
else()
  # Populating CMAKE_CONFIGURATION_TYPES even for single config generators is
  # common enough for user projects that we don't want to consider it an error.
  # We just need CMAKE_BUILD_TYPE to be set and ignore CMAKE_CONFIGURATION_TYPES.
  if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE is not defined or is empty "
        "(but should be defined and non-empty for a single-configuration generator)")
  endif()
  add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
  set(_configs ${CMAKE_BUILD_TYPE})
endif()

add_executable(ctc CTestConfig.cxx)


foreach(cfg ${_configs})
  add_test(NAME ctc-${cfg} CONFIGURATIONS ${cfg} COMMAND ctc --config $<CONFIGURATION>)

  if(_isMultiConfig)
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

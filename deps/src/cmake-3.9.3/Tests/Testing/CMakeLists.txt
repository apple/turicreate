#
# Testing
#
cmake_minimum_required (VERSION 2.7)
project (Testing)

#
# Lib and exe path
#
set (LIBRARY_OUTPUT_PATH
     ${Testing_BINARY_DIR}/bin/ CACHE PATH
     "Single output directory for building all libraries.")

set (EXECUTABLE_OUTPUT_PATH
     ${Testing_BINARY_DIR}/bin/ CACHE PATH
     "Single output directory for building all executables.")

#
# Where will executable tests be written ?
#
if (EXECUTABLE_OUTPUT_PATH)
  set (CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})
else ()
  set (CXX_TEST_PATH .)
endif ()

#
# Include Dart
# (will also set NSLOOKUP, HOSTNAME, etc.)
#
include (${CMAKE_ROOT}/Modules/Dart.cmake)

#
# Extra coverage
#
build_command(BUILD_COMMAND_VAR ${CMAKE_MAKE_PROGRAM})
build_name(BUILD_NAME_VAR)
site_name(SITE_NAME_VAR)

#
# Enable testing
#
enable_testing()

#
# Add test
#
add_executable(testing testing.cxx)
add_test(testing.1 ${Testing_BINARY_DIR}/bin/testing)

#
# skip level test
#
add_subdirectory(Sub/Sub2)

# Per-config target name and generator expressions.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../PerConfig PerConfig)
add_test(NAME testing.perconfig COMMAND perconfig)
add_test(NAME testing.driver COMMAND ${PerConfig_COMMAND})

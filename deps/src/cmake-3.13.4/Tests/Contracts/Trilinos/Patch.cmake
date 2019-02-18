if(NOT DEFINED source_dir)
  message(FATAL_ERROR "variable 'source_dir' not defined")
endif()


if(NOT EXISTS "${source_dir}/CMakeLists.txt")
  message(FATAL_ERROR "error: No CMakeLists.txt file to patch!")
endif()

set(text "

#
# Reference variables typically given as experimental_build_test configure
# options to avoid CMake warnings about unused variables
#

message(\"Trilinos_ALLOW_NO_PACKAGES='\${Trilinos_ALLOW_NO_PACKAGES}'\")
message(\"Trilinos_WARNINGS_AS_ERRORS_FLAGS='\${Trilinos_WARNINGS_AS_ERRORS_FLAGS}'\")
")

file(APPEND "${source_dir}/CMakeLists.txt" "${text}")


if(NOT EXISTS "${source_dir}/CTestConfig.cmake")
  message(FATAL_ERROR "error: No CTestConfig.cmake file to patch!")
endif()

set(text "

#
# Use newer than 10.6.1 CTestConfig settings from the Trilinos project.
# Send the Trilinos dashboards to the new Trilinos CDash server instance.
#
set(CTEST_NIGHTLY_START_TIME \"04:00:00 UTC\") # 10 PM MDT or 9 PM MST
set(CTEST_DROP_SITE \"testing.sandia.gov\")
")

file(APPEND "${source_dir}/CTestConfig.cmake" "${text}")

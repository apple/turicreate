if(NOT DEFINED CTEST_CONFIGURATION_TYPE)
  message(FATAL_ERROR "expected variable CTEST_CONFIGURATION_TYPE not defined")
endif()

if(NOT DEFINED dir)
  message(FATAL_ERROR "expected variable dir not defined")
endif()

if(NOT DEFINED gen)
  message(FATAL_ERROR "expected variable gen not defined")
endif()

message(STATUS "CTEST_CONFIGURATION_TYPE='${CTEST_CONFIGURATION_TYPE}'")
message(STATUS "dir='${dir}'")
message(STATUS "gen='${gen}'")

if(gen STREQUAL "Xcode")
  set(expected_filename "${dir}/${CTEST_CONFIGURATION_TYPE}/CFBundleTest.plugin/Contents/MacOS/CFBundleTest")
else()
  set(expected_filename "${dir}/CFBundleTest.plugin/Contents/MacOS/CFBundleTest")
endif()

if(NOT EXISTS "${expected_filename}")
  message(FATAL_ERROR "test fails: expected output file does not exist [${expected_filename}]")
endif()

file(COPY "${expected_filename}"
  DESTINATION "${dir}/LatestBuildResult"
  )

include(${RunCMake_SOURCE_DIR}/check.cmake)

file(GLOB INFO_FILES ${RunCMake_TEST_BINARY_DIR}/runtest_info_*.cmake)

if(NOT INFO_FILES)
  message(FATAL_ERROR "missing expected info files")
endif()

foreach(INFO_FILE IN LISTS INFO_FILES)
  include(${INFO_FILE})
  include(${RunCMake_TEST_BINARY_DIR}/CPackProperties.cmake)
  test_property("config.cpp" FOO ${EXPECTED_MYTEST_NAME})
endforeach()

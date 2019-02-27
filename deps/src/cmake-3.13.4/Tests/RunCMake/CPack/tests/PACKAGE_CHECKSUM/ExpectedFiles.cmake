set(EXPECTED_FILES_COUNT "0")

if(NOT ${RunCMake_SUBTEST_SUFFIX} MATCHES "invalid")
  set(EXPECTED_FILES_COUNT "1")
  set(EXPECTED_FILE_CONTENT_1_LIST "/foo;/foo/CMakeLists.txt")
endif()

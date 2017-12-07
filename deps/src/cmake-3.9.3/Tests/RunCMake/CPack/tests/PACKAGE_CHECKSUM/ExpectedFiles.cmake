set(EXPECTED_FILES_COUNT "0")

if(NOT ${RunCMake_SUBTEST_SUFFIX} MATCHES "invalid")
  set(EXPECTED_FILES_COUNT "1")
  set(EXPECTED_FILE_CONTENT_1_LIST "/usr;/usr/foo;/usr/foo/CMakeLists.txt")
endif()

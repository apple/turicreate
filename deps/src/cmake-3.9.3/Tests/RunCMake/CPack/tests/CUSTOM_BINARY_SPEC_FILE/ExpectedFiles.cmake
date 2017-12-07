set(EXPECTED_FILES_COUNT "1")
set(EXPECTED_FILE_CONTENT_1_LIST "/usr;/usr/foo;/usr/foo/CMakeLists.txt")

if(PACKAGING_TYPE STREQUAL "COMPONENT")
  set(EXPECTED_FILES_COUNT "2")
  set(EXPECTED_FILE_1_COMPONENT "test")
  set(EXPECTED_FILE_2_COMPONENT "test2")
  set(EXPECTED_FILE_CONTENT_2_LIST "/usr;/usr/bar;/usr/bar/CMakeLists.txt")
endif()

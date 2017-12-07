set(EXPECTED_FILES_COUNT "1")

set(EXPECTED_FILE_CONTENT_1_LIST
  "/usr"
  "/usr/empty_dir"
  "/usr/non_empty_dir"
  "/usr/non_empty_dir/CMakeLists.txt"
  "/usr/symlink_to_empty_dir"
  "/usr/symlink_to_non_empty_dir")

if(PACKAGING_TYPE STREQUAL "COMPONENT")
  set(EXPECTED_FILE_1_COMPONENT "links")
endif()

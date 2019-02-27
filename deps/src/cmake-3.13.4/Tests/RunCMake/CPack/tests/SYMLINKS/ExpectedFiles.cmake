set(EXPECTED_FILES_COUNT "1")

set(EXPECTED_FILE_CONTENT_1_LIST
  "/empty_dir"
  "/non_empty_dir"
  "/non_empty_dir/CMakeLists.txt"
  "/symlink_to_empty_dir"
  "/symlink_to_non_empty_dir")

if(PACKAGING_TYPE STREQUAL "COMPONENT")
  set(EXPECTED_FILE_1_COMPONENT "links")
endif()

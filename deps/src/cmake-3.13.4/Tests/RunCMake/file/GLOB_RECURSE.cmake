file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test/dir 1/empty_dir")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test/dir 1/non_empty_dir")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test/dir 2/empty_dir")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test/dir 2/non_empty_dir")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test/dir 1/dir 1 file" "test file")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test/dir 1/non_empty_dir/dir 1 subdir file" "test file")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test/dir 2/dir 2 file" "test file")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test/dir 2/non_empty_dir/dir 2 subdir file" "test file")

file(GLOB_RECURSE CONTENT_LIST "${CMAKE_CURRENT_BINARY_DIR}/test/*/*")
list(LENGTH CONTENT_LIST CONTENT_COUNT)
message("content: ${CONTENT_COUNT} ")
message("${CONTENT_LIST}")

file(GLOB_RECURSE CONTENT_LIST LIST_DIRECTORIES false "${CMAKE_CURRENT_BINARY_DIR}/test/*/*")
list(LENGTH CONTENT_LIST CONTENT_COUNT)
message("content: ${CONTENT_COUNT} ")
message("${CONTENT_LIST}")

file(GLOB_RECURSE CONTENT_LIST LIST_DIRECTORIES true "${CMAKE_CURRENT_BINARY_DIR}/test/*/*")
list(LENGTH CONTENT_LIST CONTENT_COUNT)
message("content: ${CONTENT_COUNT} ")
message("${CONTENT_LIST}")

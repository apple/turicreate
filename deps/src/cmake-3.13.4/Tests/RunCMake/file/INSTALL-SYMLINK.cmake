set(src "${CMAKE_CURRENT_BINARY_DIR}/src")
set(dst "${CMAKE_CURRENT_BINARY_DIR}/dst")
file(REMOVE RECURSE "${src}")
file(REMOVE RECURSE "${dst}")

file(MAKE_DIRECTORY "${src}")
execute_process(COMMAND
  ${CMAKE_COMMAND} -E create_symlink source "${src}/current_dir_symlink")

message(STATUS "Before Installing")
file(INSTALL FILES "${src}/current_dir_symlink"
  DESTINATION ${dst} TYPE DIRECTORY)
message(STATUS "After Installing")

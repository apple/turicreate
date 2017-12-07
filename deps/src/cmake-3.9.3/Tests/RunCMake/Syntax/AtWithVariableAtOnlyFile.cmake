set(right "wrong")
set(var "\${right}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/atfile.txt.in"
  "${CMAKE_CURRENT_BINARY_DIR}/atfile.txt"
  @ONLY)
file(READ "${CMAKE_CURRENT_BINARY_DIR}/atfile.txt" output)
message("-->${output}<--")

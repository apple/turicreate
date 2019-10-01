configure_file(Relative-In.txt Relative-Out.txt)
file(READ ${CMAKE_CURRENT_BINARY_DIR}/Relative-Out.txt out)
message("${out}")

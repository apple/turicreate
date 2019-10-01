file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/DirOutput)
configure_file(DirOutput.txt DirOutput)
file(READ ${CMAKE_CURRENT_BINARY_DIR}/DirOutput/DirOutput.txt out)
message("${out}")

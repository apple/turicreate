file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/relative-input-WARN.txt "relative-input-WARN\n")
file(GENERATE OUTPUT relative-output-WARN.txt INPUT relative-input-WARN.txt)

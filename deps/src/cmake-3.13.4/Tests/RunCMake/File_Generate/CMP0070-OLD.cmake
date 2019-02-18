cmake_policy(SET CMP0070 OLD)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/relative-input-OLD.txt "relative-input-OLD\n")
file(GENERATE OUTPUT relative-output-OLD.txt INPUT relative-input-OLD.txt)

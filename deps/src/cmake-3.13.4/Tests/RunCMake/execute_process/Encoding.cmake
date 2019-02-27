execute_process(
  COMMAND ${TEST_ENCODING_EXE} ${TEST_ENCODING} ${CMAKE_CURRENT_LIST_DIR}/EncodingUTF8-stderr.txt
  OUTPUT_VARIABLE out
  ENCODING ${TEST_ENCODING}
  )
message("${out}")

execute_process(
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/MergeOutput.cmake
  OUTPUT_FILE out.txt
  ERROR_FILE out.txt
  )
file(READ out.txt out)
message("${out}")

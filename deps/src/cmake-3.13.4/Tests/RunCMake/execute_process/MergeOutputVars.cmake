execute_process(
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/MergeOutput.cmake
  OUTPUT_VARIABLE out
  ERROR_VARIABLE out
  )
message("${out}")

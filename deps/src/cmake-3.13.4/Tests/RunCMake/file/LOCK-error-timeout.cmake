set(script "${CMAKE_CURRENT_LIST_DIR}/timeout-script.cmake")
set(file_to_lock "${CMAKE_CURRENT_BINARY_DIR}/file-to-lock")

file(LOCK "${file_to_lock}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-Dfile_to_lock=${file_to_lock}" -P "${script}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

message("Output: ${output}")
message("Error: ${error}")

if(NOT result EQUAL 0)
  message(FATAL_ERROR "Result: ${result}")
endif()

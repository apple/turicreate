message("Running CMake on RerunCMake") # write to stderr if cmake reruns
configure_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ConfigureFileInput.txt.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ConfigureFileOutput.txt"
  @ONLY
  )
# make sure CMakeCache.txt is newer than ConfigureFileOutput.txt
execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 1)

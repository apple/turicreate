message("Running CMake on RerunCMake") # write to stderr if cmake reruns
enable_language(C)
try_compile(res
  "${CMAKE_CURRENT_BINARY_DIR}"
  SOURCES "${CMAKE_CURRENT_BINARY_DIR}/TryCompileInput.c"
  )
message("${res}")

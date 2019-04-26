add_custom_command(
  OUTPUT hello.copy.c
  COMMAND "${CMAKE_COMMAND}" -E copy
          "${CMAKE_CURRENT_SOURCE_DIR}/hello.c"
          hello.copy.c
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPFILE "test.d"
  )

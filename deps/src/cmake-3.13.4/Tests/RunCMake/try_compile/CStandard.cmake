enable_language(C)
try_compile(result ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  C_STANDARD 3
  OUTPUT_VARIABLE out
  )
message("try_compile output:\n${out}")

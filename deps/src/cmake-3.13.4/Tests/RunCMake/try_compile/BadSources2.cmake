enable_language(C)
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src.c
          ${CMAKE_CURRENT_SOURCE_DIR}/src.cxx
  )

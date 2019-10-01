enable_language(CXX)
try_compile(result ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src.cxx
  CXX_STANDARD 3 # bogus, but not used
  OUTPUT_VARIABLE out
  )
if(NOT result)
  message(FATAL_ERROR "try_compile failed:\n${out}")
endif()

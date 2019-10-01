enable_language(C)
try_compile(result ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CStandardGNU.c
  C_STANDARD 99
  C_STANDARD_REQUIRED 1
  C_EXTENSIONS 0
  OUTPUT_VARIABLE out
  )
if(NOT result)
  message(FATAL_ERROR "try_compile failed:\n${out}")
endif()

cmake_policy(SET CMP0067 NEW)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED 1)
set(CMAKE_C_EXTENSIONS 0)
try_compile(result ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CStandardGNU.c
  OUTPUT_VARIABLE out
  )
if(NOT result)
  message(FATAL_ERROR "try_compile failed:\n${out}")
endif()

enable_language(C)

set(CMAKE_WARN_DEPRECATED SOME_VALUE)

try_compile(result ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  OUTPUT_VARIABLE out
  )
if(NOT result)
  message(FATAL_ERROR "try_compile failed:\n${out}")
endif()

# Check that the cache was populated with our custom variable.
file(STRINGS ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/CMakeCache.txt entries
  REGEX CMAKE_WARN_DEPRECATED:UNINITIALIZED=${CMAKE_WARN_DEPRECATED}
  )
if(NOT entries)
  message(FATAL_ERROR "try_compile did not populate cache as expected")
endif()

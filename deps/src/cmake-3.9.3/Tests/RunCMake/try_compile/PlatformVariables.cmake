enable_language(C)

# Normally this variable should be set by a platform information module or
# a toolchain file, but for purposes of this test we simply set it here.
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES MY_CUSTOM_VARIABLE)

set(MY_CUSTOM_VARIABLE SOME_VALUE)

try_compile(result ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  OUTPUT_VARIABLE out
  )
if(NOT result)
  message(FATAL_ERROR "try_compile failed:\n${out}")
endif()

# Check that the cache was populated with our custom variable.
file(STRINGS ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/CMakeCache.txt entries
  REGEX MY_CUSTOM_VARIABLE:UNINITIALIZED=${MY_CUSTOM_VARIABLE}
  )
if(NOT entries)
  message(FATAL_ERROR "try_compile did not populate cache as expected")
endif()

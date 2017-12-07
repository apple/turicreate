include(Compiler/Clang)
__compiler_clang(C)

cmake_policy(GET CMP0025 appleClangPolicy)
if(WIN32 OR (APPLE AND NOT appleClangPolicy STREQUAL NEW))
  return()
endif()

if(NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 3.4)
  set(CMAKE_C90_STANDARD_COMPILE_OPTION "-std=c90")
  set(CMAKE_C90_EXTENSION_COMPILE_OPTION "-std=gnu90")

  set(CMAKE_C99_STANDARD_COMPILE_OPTION "-std=c99")
  set(CMAKE_C99_EXTENSION_COMPILE_OPTION "-std=gnu99")

  set(CMAKE_C11_STANDARD_COMPILE_OPTION "-std=c11")
  set(CMAKE_C11_EXTENSION_COMPILE_OPTION "-std=gnu11")
endif()

__compiler_check_default_language_standard(C 3.4 99 3.6 11)

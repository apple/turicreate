cmake_minimum_required(VERSION 2.8.12)
cmake_policy(SET CMP0025 NEW)
project(CxxDialect)

add_executable(use_typeof use_typeof.cxx)
set_property(TARGET use_typeof PROPERTY CXX_STANDARD 98)

add_executable(use_constexpr use_constexpr.cxx)
set_property(TARGET use_constexpr PROPERTY CXX_STANDARD 11)

add_executable(CxxDialect use_constexpr_and_typeof.cxx)
set_property(TARGET CxxDialect PROPERTY CXX_STANDARD 11)

try_compile(typeof_no_extensions_works
  "${CMAKE_CURRENT_BINARY_DIR}/use_typeof_test"
  "${CMAKE_CURRENT_SOURCE_DIR}/use_typeof.cxx"
  COMPILE_DEFINITIONS
  CMAKE_FLAGS
    "-DCMAKE_CXX_STANDARD=98"
    "-DCMAKE_CXX_EXTENSIONS=FALSE"
  OUTPUT_VARIABLE OUTPUT
)

if (typeof_no_extensions_works)
  message("Use of typeof extension with extensions disabled works, but is expected to fail: ${OUTPUT}")
else()
  message("Use of typeof extension with extensions disabled fails, as expected")
endif()

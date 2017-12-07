cmake_minimum_required(VERSION 2.8)

project(CompileOptions)

add_library(testlib other.cpp)

add_executable(CompileOptions main.cpp)

macro(get_compiler_test_genex lst lang)
  list(APPEND ${lst} -DTEST_${lang}_COMPILER_VERSION="$<${lang}_COMPILER_VERSION>")
  list(APPEND ${lst} -DTEST_${lang}_COMPILER_VERSION_EQUALITY=$<${lang}_COMPILER_VERSION:${CMAKE_${lang}_COMPILER_VERSION}>)
endmacro()

get_compiler_test_genex(c_tests C)
get_compiler_test_genex(cxx_tests CXX)

set_property(TARGET CompileOptions PROPERTY COMPILE_OPTIONS
  "-DTEST_DEFINE"
  "-DNEEDS_ESCAPE=\"E$CAPE\""
  "$<$<CXX_COMPILER_ID:GNU>:-DTEST_DEFINE_GNU>"
  ${c_tests}
  ${cxx_tests}
  )

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|Borland|Embarcadero")
  set_property(TARGET CompileOptions APPEND PROPERTY COMPILE_OPTIONS
    "-DTEST_OCTOTHORPE=\"#\""
    )
endif()

target_link_libraries(CompileOptions testlib)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_definitions(CompileOptions
    PRIVATE
      "DO_GNU_TESTS"
  )
endif()

target_compile_definitions(CompileOptions
  PRIVATE
    "EXPECTED_C_COMPILER_VERSION=\"${CMAKE_C_COMPILER_VERSION}\""
    "EXPECTED_CXX_COMPILER_VERSION=\"${CMAKE_CXX_COMPILER_VERSION}\""
)

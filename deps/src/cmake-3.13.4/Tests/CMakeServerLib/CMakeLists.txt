include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMake_BINARY_DIR}/Source
  ${CMake_SOURCE_DIR}/Source
  )

set(CMakeServerLib_TESTS
  testServerBuffering.cpp
  )

create_test_sourcelist(CMakeLib_TEST_SRCS CMakeServerLibTests.cxx ${CMakeServerLib_TESTS})
add_executable(CMakeServerLibTests ${CMakeLib_TEST_SRCS})
target_link_libraries(CMakeServerLibTests CMakeLib CMakeServerLib)

SET_PROPERTY(TARGET CMakeServerLibTests PROPERTY C_CLANG_TIDY "")
SET_PROPERTY(TARGET CMakeServerLibTests PROPERTY CXX_CLANG_TIDY "")

foreach(testfile ${CMakeServerLib_TESTS})
  get_filename_component(test "${testfile}" NAME_WE)
  add_test(CMakeServerLib.${test} CMakeServerLibTests ${test} ${${test}_ARGS})
endforeach()

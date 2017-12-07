file(READ ${RunCMake_TEST_BINARY_DIR}/foo.txt foo_sources)

# VS generators inject CMakeLists.txt as a source.  Remove it.
string(REGEX REPLACE ";[^;]*CMakeLists.txt$" "" foo_sources "${foo_sources}")

set(foo_expected "empty.c;empty2.c;empty3.c")
if(NOT foo_sources STREQUAL foo_expected)
  set(RunCMake_TEST_FAILED "foo SOURCES was:\n [[${foo_sources}]]\nbut expected:\n [[${foo_expected}]]")
endif()

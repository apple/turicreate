cmake_minimum_required (VERSION 2.6)
project(CTestTestDepends)
include(CTest)

add_executable (simple simple.cxx)
add_executable (TestExe::Simple ALIAS simple)
add_test (NAME one COMMAND TestExe::Simple)
add_test (two simple)
add_test (three simple)

# Add redundant (but not cyclical) dependencies
set_tests_properties(two PROPERTIES DEPENDS "one")
set_tests_properties(three PROPERTIES DEPENDS "one;two")

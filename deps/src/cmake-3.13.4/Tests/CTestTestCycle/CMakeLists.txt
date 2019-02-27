cmake_minimum_required (VERSION 2.6)
project(CTestTestCycle)
include(CTest)

add_executable (simple simple.cxx)
add_test (one simple)
add_test (two simple)
add_test (three simple)

# Add cyclical test dependency
set_tests_properties(one PROPERTIES DEPENDS "two")
set_tests_properties(two PROPERTIES DEPENDS "three")
set_tests_properties(three PROPERTIES DEPENDS "one")

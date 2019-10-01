cmake_minimum_required (VERSION 2.6)
project(CTestTestStopTime)
include(CTest)

add_executable (Sleep sleep.c)

add_test (TestSleep Sleep 30)
add_test (ShouldNotRun Sleep 30)

set_tests_properties(ShouldNotRun PROPERTIES DEPENDS TestSleep)
set_tests_properties(ShouldNotRun PROPERTIES WILL_FAIL ON)

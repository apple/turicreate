cmake_minimum_required (VERSION 2.6)
project(CTestTestCrash)
include(CTest)

add_executable (Crash crash.cxx)

add_test (TestCrash Crash)

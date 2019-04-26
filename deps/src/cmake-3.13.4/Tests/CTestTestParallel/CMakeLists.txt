cmake_minimum_required (VERSION 2.6)
project(CTestTestParallel)
include(CTest)

add_executable (LockFile lockFile.c)

add_test (TestRunSerial1 LockFile)
add_test (TestRunSerial2 LockFile)
set_tests_properties(TestRunSerial1 TestRunSerial2 PROPERTIES RUN_SERIAL true)

add_test (TestProcessorsGreaterThanMPL1 LockFile)
add_test (TestProcessorsGreaterThanMPL2 LockFile)
set_tests_properties(TestProcessorsGreaterThanMPL1 PROPERTIES PROCESSORS 10)
set_tests_properties(TestProcessorsGreaterThanMPL1 PROPERTIES DEPENDS
                     TestProcessorsGreaterThanMPL2)

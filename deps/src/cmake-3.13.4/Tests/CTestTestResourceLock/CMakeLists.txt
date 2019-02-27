cmake_minimum_required (VERSION 2.6)
project(CTestTestResourceLock)
include(CTest)

add_executable (LockFile lockFile.c)

add_test (TestLockedFile1.1 LockFile locked1.txt)
add_test (TestLockedFile1.2 LockFile locked1.txt)
set_tests_properties(TestLockedFile1.1 TestLockedFile1.2 PROPERTIES RESOURCE_LOCK "locked1.txt")

add_test (TestLockedFile2.1 LockFile locked2.txt)
add_test (TestLockedFile2.2 LockFile locked2.txt)
set_tests_properties(TestLockedFile2.1 TestLockedFile2.2 PROPERTIES RESOURCE_LOCK "locked2.txt")

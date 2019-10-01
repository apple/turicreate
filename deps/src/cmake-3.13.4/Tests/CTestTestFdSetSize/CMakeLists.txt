cmake_minimum_required (VERSION 2.8.10)
project (CTestTestFdSetSize)
include (CTest)

add_executable (Sleep sleep.c)

foreach (index RANGE 1 20)
  add_test (TestSleep${index} Sleep)
endforeach ()

cmake_minimum_required (VERSION 2.6)
project (CTestTestCostSerial)
include (CTest)

add_executable (Sleep sleep.c)

foreach (index RANGE 1 3)
  add_test (TestSleep${index} Sleep)
endforeach ()

set_tests_properties(TestSleep1 PROPERTIES COST -500)
set_tests_properties(TestSleep2 PROPERTIES COST 12)
set_tests_properties(TestSleep3 PROPERTIES COST 0)

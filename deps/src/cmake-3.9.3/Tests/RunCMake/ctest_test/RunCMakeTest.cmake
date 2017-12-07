include(RunCTest)
set(RunCMake_TEST_TIMEOUT 60)

set(CASE_CTEST_TEST_ARGS "")
set(CASE_CTEST_TEST_LOAD "")

function(run_ctest_test CASE_NAME)
  set(CASE_CTEST_TEST_ARGS "${ARGN}")
  run_ctest(${CASE_NAME})
endfunction()

run_ctest_test(TestQuiet QUIET)

# Tests for the 'Test Load' feature of ctest
#
# Spoof a load average value to make these tests more reliable.
set(ENV{__CTEST_FAKE_LOAD_AVERAGE_FOR_TESTING} 5)

# Verify that new tests are started when the load average falls below
# our threshold.
run_ctest_test(TestLoadPass TEST_LOAD 6)

# Verify that new tests are not started when the load average exceeds
# our threshold.
run_ctest_test(TestLoadFail TEST_LOAD 2)

# Verify that when an invalid "TEST_LOAD" value is given, a warning
# message is displayed and the value is ignored.
run_ctest_test(TestLoadInvalid TEST_LOAD "ERR1")

# Verify that new tests are started when the load average falls below
# our threshold.
set(CASE_CTEST_TEST_LOAD 7)
run_ctest_test(CTestTestLoadPass)

# Verify that new tests are not started when the load average exceeds
# our threshold.
set(CASE_CTEST_TEST_LOAD 4)
run_ctest_test(CTestTestLoadFail)

# Verify that when an invalid "CTEST_TEST_LOAD" value is given,
# a warning message is displayed and the value is ignored.
set(CASE_CTEST_TEST_LOAD "ERR2")
run_ctest_test(CTestTestLoadInvalid)

# Verify that the "TEST_LOAD" value has higher precedence than
# the "CTEST_TEST_LOAD" value
set(CASE_CTEST_TEST_LOAD "ERR3")
run_ctest_test(TestLoadOrder TEST_LOAD "ERR4")

unset(ENV{__CTEST_FAKE_LOAD_AVERAGE_FOR_TESTING})
unset(CASE_CTEST_TEST_LOAD)

function(run_TestChangeId)
  set(CASE_TEST_PREFIX_CODE [[
    set(CTEST_CHANGE_ID "<>1")
  ]])

  run_ctest(TestChangeId)
endfunction()
run_TestChangeId()

function(run_TestOutputSize)
  set(CASE_CTEST_TEST_ARGS EXCLUDE RunCMakeVersion)
  set(CASE_TEST_PREFIX_CODE [[
set(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE 10)
set(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE 12)
  ]])
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_test(NAME PassingTest COMMAND ${CMAKE_COMMAND} -E echo PassingTestOutput)
add_test(NAME FailingTest COMMAND ${CMAKE_COMMAND} -E no_such_command)
  ]])

  unset(ENV{CTEST_PARALLEL_LEVEL})
  run_ctest(TestOutputSize)
endfunction()
run_TestOutputSize()

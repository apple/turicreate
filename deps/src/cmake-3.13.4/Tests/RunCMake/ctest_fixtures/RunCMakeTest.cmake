include(RunCTest)

# Isolate our ctest runs from external environment.
unset(ENV{CTEST_PARALLEL_LEVEL})
unset(ENV{CTEST_OUTPUT_ON_FAILURE})

function(run_ctest_test CASE_NAME)
  set(CASE_CTEST_FIXTURES_ARGS "${ARGN}")
  run_ctest(${CASE_NAME})
endfunction()

#------------------------------------------------------------
# CMake configure will pass
#------------------------------------------------------------
run_ctest_test(one      INCLUDE one)
run_ctest_test(two      INCLUDE two)
run_ctest_test(three    INCLUDE three)
run_ctest_test(setupFoo INCLUDE setupFoo)
run_ctest_test(wontRun  INCLUDE wontRun)
run_ctest_test(unused   INCLUDE Unused)

run_ctest_test(exclude_setup_foo
    INCLUDE               "one|two"
    EXCLUDE_FIXTURE_SETUP "Foo"
)

run_ctest_test(exclude_setup_bar
    INCLUDE               "one|two"
    EXCLUDE_FIXTURE_SETUP "Bar"
)

run_ctest_test(exclude_cleanup_foo
    INCLUDE                 "one|two"
    EXCLUDE_FIXTURE_CLEANUP "Foo"
)

run_ctest_test(exclude_cleanup_bar
    INCLUDE                 "one|two"
    EXCLUDE_FIXTURE_CLEANUP "Bar"
)

run_ctest_test(exclude_any_foo
    INCLUDE         "one|two"
    EXCLUDE_FIXTURE "Foo"
)

run_ctest_test(exclude_any_bar
    INCLUDE         "one|two"
    EXCLUDE_FIXTURE "Bar"
)

run_ctest_test(exclude_any_foobar
    INCLUDE         "one|two"
    EXCLUDE_FIXTURE "Foo|Bar"
)

#------------------------------------------------------------
# CMake configure will fail due to cyclic test dependencies
#------------------------------------------------------------
set(CASE_CMAKELISTS_CYCLIC_CODE [[
    set_tests_properties(cyclicSetup PROPERTIES
                         FIXTURES_SETUP    "Foo"
                         FIXTURES_REQUIRED "Foo")
]])
run_ctest(cyclicSetup)

set(CASE_CMAKELISTS_CYCLIC_CODE [[
    set_tests_properties(cyclicCleanup PROPERTIES
                         FIXTURES_CLEANUP  "Foo"
                         FIXTURES_REQUIRED "Foo")
]])
run_ctest(cyclicCleanup)

#------------------------------------------------------------
# Repeat some of the exclusion tests with ctest command line
# options instead of arguments to ctest_test(). This verifies
# that the command line options make it through as well.
#------------------------------------------------------------
unset(CASE_CMAKELISTS_CYCLIC_CODE)
set(CASE_CTEST_FIXTURES_ARGS "")

run_ctest(exclude_setup_foo -R "one|two" -FS Foo)
run_ctest(exclude_setup_foo -R "one|two" --fixture-exclude-setup Foo)
run_ctest(exclude_cleanup_foo -R "one|two" -FC Foo)
run_ctest(exclude_cleanup_foo -R "one|two" --fixture-exclude-cleanup Foo)
run_ctest(exclude_any_foo -R "one|two" -FA Foo)
run_ctest(exclude_any_foo -R "one|two" --fixture-exclude-any Foo)

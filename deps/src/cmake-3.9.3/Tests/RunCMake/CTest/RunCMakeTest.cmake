include(RunCMake)

set(RunCMake_TEST_OPTIONS -DNoProject=1)
run_cmake(BeforeProject)
unset(RunCMake_TEST_OPTIONS)

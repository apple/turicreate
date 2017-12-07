include(RunCTest)

set(CASE_CTEST_START_ARGS "")

function(run_ctest_start CASE_NAME)
  set(CASE_CTEST_START_ARGS "${ARGN}")
  run_ctest(${CASE_NAME})
endfunction()

run_ctest_start(StartQuiet Experimental QUIET)

run_ctest_start(ConfigInSource Experimental)

function(run_ConfigInBuild)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/ConfigInBuild-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  configure_file(${RunCMake_SOURCE_DIR}/CTestConfig.cmake.in
                 ${RunCMake_BINARY_DIR}/ConfigInBuild-build/CTestConfig.cmake @ONLY)
  run_ctest_start(ConfigInBuild Experimental)
endfunction()
run_ConfigInBuild()

include(RunCTest)

set(CASE_CTEST_CONFIGURE_ARGS "")

function(run_ctest_configure CASE_NAME)
  set(CASE_CTEST_CONFIGURE_ARGS "${ARGN}")
  run_ctest(${CASE_NAME})
endfunction()

run_ctest_configure(ConfigureQuiet QUIET)

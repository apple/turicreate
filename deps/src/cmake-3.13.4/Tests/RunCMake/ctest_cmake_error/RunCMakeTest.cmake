include(RunCTest)

set(CASE_CTEST_COVERAGE_ARGS "")

function(run_ctest_coverage CASE_NAME)
  set(CASE_CTEST_COVERAGE_ARGS "${ARGN}")
  run_ctest(${CASE_NAME})
endfunction()

run_ctest_coverage(CTestCaptureErrorNonZero junk CAPTURE_CMAKE_ERROR val)

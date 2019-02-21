include(RunCTest)

function(run_ctest_TimeoutAfterMatch CASE_NAME)
  set(CASE_PROPERTY_ARGS "${ARGN}")
  run_ctest(${CASE_NAME})
endfunction()

run_ctest_TimeoutAfterMatch(MissingArg1 "\"-Darg2=Test started\"")
run_ctest_TimeoutAfterMatch(MissingArg2 "\"-Darg1=2\"")
run_ctest_TimeoutAfterMatch(ShouldTimeout "\"-Darg1=1\" \"-Darg2=Test started\"")
run_ctest_TimeoutAfterMatch(ShouldPass "\"-Darg1=15\" \"-Darg2=Test started\"")

include(RunCTest)

set(CASE_CTEST_UPLOAD_ARGS "")

function(run_ctest_upload CASE_NAME)
  set(CASE_CTEST_UPLOAD_ARGS "${ARGN}")
  run_ctest(${CASE_NAME})
endfunction()

run_ctest_upload(UploadQuiet QUIET)


if (NOT actual_stdout MATCHES "BADFLAG")
  set (RunCMake_TEST_FAILED "Not found expected 'BADFLAG'.")
endif()

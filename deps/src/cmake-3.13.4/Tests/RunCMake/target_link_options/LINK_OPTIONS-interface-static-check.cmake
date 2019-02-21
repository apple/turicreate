
if (NOT actual_stdout MATCHES "BADFLAG_INTERFACE")
  set (RunCMake_TEST_FAILED "Not found expected 'BADFLAG_INTERFACE'.")
endif()

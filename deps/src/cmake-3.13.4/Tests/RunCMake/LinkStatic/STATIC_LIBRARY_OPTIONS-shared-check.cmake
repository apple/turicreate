
if (actual_stdout MATCHES "BADFLAG")
  string (APPEND RunCMake_TEST_FAILED "\nFound unexpected flag 'BADFLAG'.")
endif()


if (NOT actual_stdout MATCHES "BADFLAG_PRIVATE")
  set (RunCMake_TEST_FAILED "Not found expected 'BADFLAG_PRIVATE'.")
endif()
if (actual_stdout MATCHES "BADFLAG_INTERFACE")
  string (APPEND RunCMake_TEST_FAILED "\nFound unexpected 'BADFLAG_INTERFACE'.")
endif()

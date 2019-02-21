
if (NOT actual_stdout MATCHES "BADFLAG_RELEASE")
  set (RunCMake_TEST_FAILED "Not found expected 'BADFLAG_RELEASE'.")
endif()
if (actual_stdout MATCHES "SHELL:")
  string (APPEND RunCMake_TEST_FAILED "\nFound unexpected prefix 'SHELL:'.")
endif()

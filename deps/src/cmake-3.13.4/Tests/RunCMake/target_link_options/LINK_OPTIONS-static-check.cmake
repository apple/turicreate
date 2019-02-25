
if (actual_stdout MATCHES "BADFLAG_RELEASE")
  set (RunCMake_TEST_FAILED "Found 'BADFLAG_RELEASE' which was not expected.")
endif()
if (actual_stdout MATCHES "SHELL:")
  string (APPEND RunCMake_TEST_FAILED "\nFound unexpected prefix 'SHELL:'.")
endif()

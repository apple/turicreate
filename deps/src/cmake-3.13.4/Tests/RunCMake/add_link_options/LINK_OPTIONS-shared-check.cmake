
if (NOT actual_stdout MATCHES "BADFLAG_SHARED_RELEASE")
  set (RunCMake_TEST_FAILED "Not found expected 'BADFLAG_SHARED_RELEASE'.")
endif()
if (actual_stdout MATCHES "BADFLAG_(MODULE|EXECUTABLE)_RELEASE")
  set (RunCMake_TEST_FAILED "Found unexpected 'BADFLAG_(MODULE|EXECUTABLE)_RELEASE'.")
endif()

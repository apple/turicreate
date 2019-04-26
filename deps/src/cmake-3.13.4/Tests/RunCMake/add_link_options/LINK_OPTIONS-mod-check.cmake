
if (NOT actual_stdout MATCHES "BADFLAG_MODULE_RELEASE")
  set (RunCMake_TEST_FAILED "Not found expected 'BADFLAG_MODULE_RELEASE'.")
endif()
if (actual_stdout MATCHES "BADFLAG_(SHARED|EXECUTABLE)_RELEASE")
  set (RunCMake_TEST_FAILED "Found unexpected 'BADFLAG_(SHARED|EXECUTABLE)_RELEASE'.")
endif()

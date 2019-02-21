
if (NOT actual_stdout MATCHES "BADFLAG_EXECUTABLE_RELEASE")
  set (RunCMake_TEST_FAILED "Not found expected 'BADFLAG_EXECUTABLE_RELEASE'.")
endif()
if (actual_stdout MATCHES "BADFLAG_(SHARED|MODULE)_RELEASE")
  set (RunCMake_TEST_FAILED "Found unexpected 'BADFLAG_(SHARED|MODULE)_RELEASE'.")
endif()

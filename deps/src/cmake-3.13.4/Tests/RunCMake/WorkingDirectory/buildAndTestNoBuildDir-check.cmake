if(EXISTS ${RunCMake_TEST_BINARY_DIR}/CMakeCache.txt)
  set(RunCMake_TEST_FAILED "Default build dir ${RunCMake_TEST_BINARY_DIR} was used, should not have been")
endif()

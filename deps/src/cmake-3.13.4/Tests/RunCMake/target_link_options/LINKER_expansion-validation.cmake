
if (actual_stdout MATCHES "LINKER:")
  set (RunCMake_TEST_FAILED "LINKER: prefix was not expanded.")
  return()
endif()

if (NOT EXISTS "${RunCMake_TEST_BINARY_DIR}/LINKER.txt")
  set (RunCMake_TEST_FAILED "${RunCMake_TEST_BINARY_DIR}/LINKER.txt: Reference file not found.")
  return()
endif()
file(READ "${RunCMake_TEST_BINARY_DIR}/LINKER.txt" linker_flag)

if (NOT actual_stdout MATCHES "${linker_flag}")
  set (RunCMake_TEST_FAILED "LINKER: was not expanded correctly.")
endif()

set(testfile "${RunCMake_TEST_BINARY_DIR}/CTestTestfile.cmake")
if(EXISTS "${testfile}")
  file(READ "${testfile}" testfile_contents)
else()
  message(FATAL_ERROR "Could not find expected CTestTestfile.cmake.")
endif()

set(error_details "There is a problem with generated test file: ${testfile}")

if(testfile_contents MATCHES "add_test[(]DoesNotUseEmulator [^\n]+pseudo_emulator[^\n]+\n")
  message(SEND_ERROR "Used emulator when it should not be used. ${error_details}")
endif()

if(NOT testfile_contents MATCHES "add_test[(]UsesEmulator [^\n]+pseudo_emulator[^\n]+\n")
  message(SEND_ERROR "Did not use emulator when it should be used. ${error_details}")
endif()

if(testfile_contents MATCHES "add_test[(]DoesNotUseEmulatorWithGenex [^\n]+pseudo_emulator[^\n]+\n")
  message(SEND_ERROR "Used emulator when it should not be used. ${error_details}")
endif()

if(NOT testfile_contents MATCHES "add_test[(]UsesEmulatorWithExecTargetFromSubdirAddedWithoutGenex [^\n]+pseudo_emulator[^\n]+\n")
  message(SEND_ERROR "Did not use emulator when it should be used. ${error_details}")
endif()

if(testfile_contents MATCHES "add_test[(]DoesNotUseEmulatorWithExecTargetFromSubdirAddedWithGenex [^\n]+pseudo_emulator[^\n]+\n")
  message(SEND_ERROR "Used emulator when it should not be used. ${error_details}")
endif()

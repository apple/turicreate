message(STATUS "=============================================================================")
message(STATUS "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)")
message(STATUS "")

if(NOT CPackComponents_BINARY_DIR)
  message(FATAL_ERROR "CPackComponents_BINARY_DIR not set")
endif()

set(expected_file_mask "")

if(WIN32)
  # Only expect the *.exe installer if it looks like NSIS is
  # installed on this machine:
  #
  find_program(NSIS_MAKENSIS_EXECUTABLE NAMES makensis
    PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\NSIS]
    DOC "makensis.exe location"
    )
  if(NSIS_MAKENSIS_EXECUTABLE)
    set(expected_file_mask "${CPackComponents_BINARY_DIR}/MyLib-*.exe")
  endif()
endif()

if(APPLE)
  # Always expect the *.dmg installer - PackageMaker should always
  # be installed on a development Mac:
  #
  set(expected_file_mask "${CPackComponents_BINARY_DIR}/MyLib-*.dmg")
endif()

if(expected_file_mask)
  set(expected_count 1)
  file(GLOB expected_file "${expected_file_mask}")

  message(STATUS "expected_count='${expected_count}'")
  message(STATUS "expected_file='${expected_file}'")
  message(STATUS "expected_file_mask='${expected_file_mask}'")

  if(NOT expected_file)
    message(FATAL_ERROR "error: expected_file does not exist: CPackComponents test fails.")
  endif()

  list(LENGTH expected_file actual_count)
  message(STATUS "actual_count='${actual_count}'")
  if(NOT actual_count EQUAL expected_count)
    message(FATAL_ERROR "error: expected_count does not match actual_count: CPackComponents test fails.")
  endif()
endif()

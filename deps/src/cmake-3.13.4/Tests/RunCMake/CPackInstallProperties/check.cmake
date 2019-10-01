function(test_property FILE NAME EXPECTED_VALUE)
  get_property(ACTUAL_VALUE INSTALL "${FILE}" PROPERTY "${NAME}")

  if(NOT "${ACTUAL_VALUE}" STREQUAL "${EXPECTED_VALUE}")
    message(FATAL_ERROR "${NAME}@${FILE}: property mismatch expected [${EXPECTED_VALUE}] actual [${ACTUAL_VALUE}] (Config:${CPACK_BUILD_CONFIG})")
  endif()
endfunction()

set(CPACK_BUILD_CONFIG Debug)
include(${RunCMake_TEST_BINARY_DIR}/CPackProperties.cmake)

include(${RunCMake_TEST_BINARY_DIR}/runtest_info.cmake OPTIONAL)

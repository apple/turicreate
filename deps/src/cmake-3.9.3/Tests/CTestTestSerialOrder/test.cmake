list(APPEND EXPECTED_OUTPUT
  initialization
  test9
  test8
  test1
  test2
  test3
  test6
  test7a
  test7b
  test5
  test4
  test10
  test11
  test12
)


if("${TEST_NAME}" STREQUAL "initialization")
  file(WRITE ${TEST_OUTPUT_FILE} "${TEST_NAME}")

elseif("${TEST_NAME}" STREQUAL "verification")
  file(READ ${TEST_OUTPUT_FILE} ACTUAL_OUTPUT)
  if(NOT "${ACTUAL_OUTPUT}" STREQUAL "${EXPECTED_OUTPUT}")
    message(FATAL_ERROR "Actual test order [${ACTUAL_OUTPUT}] differs from expected test order [${EXPECTED_OUTPUT}]")
  endif()

else()
  file(APPEND ${TEST_OUTPUT_FILE} ";${TEST_NAME}")

endif()

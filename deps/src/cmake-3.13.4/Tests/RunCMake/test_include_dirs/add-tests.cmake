set(script)

foreach(TEST_NAME ${TEST_NAMES})
  set(script "${script}add_test(\"${TEST_SUITE}.${TEST_NAME}\"")
  set(script "${script} \"${TEST_EXECUTABLE}\")\n")
endforeach()

file(WRITE "${CTEST_FILE}" "${script}")

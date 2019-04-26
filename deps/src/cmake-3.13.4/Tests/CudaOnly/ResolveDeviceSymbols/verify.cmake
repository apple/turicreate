execute_process(COMMAND ${DUMP_COMMAND} ${DUMP_ARGS} ${TEST_LIBRARY_PATH}
  RESULT_VARIABLE RESULT
  OUTPUT_VARIABLE OUTPUT
  ERROR_VARIABLE ERROR
)

if(NOT "${RESULT}" STREQUAL "0")
  message(FATAL_ERROR "${DUMP_COMMAND} failed [${RESULT}] [${OUTPUT}] [${ERROR}]")
endif()

if(NOT "${OUTPUT}" MATCHES "(cmake_device_link|device-link)")
  message(FATAL_ERROR
    "No cuda device objects found, device linking did not occur")
endif()

execute_process(
  COMMAND "${CMAKE_INSTALL_PREFIX}/bin/myexe"
  RESULT_VARIABLE MYEXE_RESULT
  OUTPUT_VARIABLE MYEXE_OUTPUT
  ERROR_VARIABLE MYEXE_ERROR
  )

if(NOT MYEXE_RESULT EQUAL "0")
  set(RunCMake_TEST_FAILED "myexe returned [${MYEXE_RESULT}], was expecting [0]")
elseif(NOT MYEXE_OUTPUT STREQUAL "")
  set(RunCMake_TEST_FAILED "myexe printed nonempty output:\n${MYEXE_OUTPUT}")
elseif(NOT MYEXE_ERROR STREQUAL "")
  set(RunCMake_TEST_FAILED "myexe printed nonempty error:\n${MYEXE_ERROR}")
endif()

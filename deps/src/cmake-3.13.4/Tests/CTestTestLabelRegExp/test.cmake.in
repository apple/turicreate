configure_file(${SOURCE_DIR}/CTestTestfile.cmake.in CTestTestfile.cmake)

function(get_test_list TEST_LIST)
  set(QUERY_COMMAND ${CMAKE_CTEST_COMMAND} -N ${ARGN})

  execute_process(COMMAND ${QUERY_COMMAND}
    RESULT_VARIABLE RESULT
	OUTPUT_VARIABLE OUTPUT
	ERROR_VARIABLE ERROR)

  if(NOT ${RESULT} STREQUAL "0")
    message(FATAL_ERROR "command [${QUERY_COMMAND}] failed: RESULT[${RESULT}] OUTPUT[${OUTPUT}] ERROR[${ERROR}]")
  endif()

  set(${TEST_LIST} "${OUTPUT}" PARENT_SCOPE)
endfunction()

function(expect_test_list EXPECTED_OUTPUT)
  get_test_list(TEST_LIST ${ARGN})

  if(NOT "${TEST_LIST}" MATCHES "${EXPECTED_OUTPUT}")
    message(FATAL_ERROR "actual output [${TEST_LIST}] does not match expected output [${EXPECTED_OUTPUT}] for given arguments [${ARGN}]")
  endif()
endfunction()

expect_test_list("test1.*test3.*Total Tests: 2" --label-regex foo)
expect_test_list("test2.*test3.*Total Tests: 2" --label-regex bar)
expect_test_list("test1.*test2.*test3.*Total Tests: 3" --label-regex foo|bar)
expect_test_list("Total Tests: 0" --label-regex baz)

expect_test_list("test2.*Total Tests: 1" --label-exclude foo)
expect_test_list("test1.*Total Tests: 1" --label-exclude bar)
expect_test_list("Total Tests: 0" --label-exclude foo|bar)
expect_test_list("test1.*test2.*test3.*Total Tests: 3" --label-exclude baz)

expect_test_list("test1.*Total Tests: 1" --label-regex foo --label-exclude bar)
expect_test_list("test2.*Total Tests: 1" --label-regex bar --label-exclude foo)

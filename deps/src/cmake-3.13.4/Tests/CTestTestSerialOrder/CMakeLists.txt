cmake_minimum_required(VERSION 2.8.12)

project(CTestTestSerialOrder)

set(TEST_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/test_output.txt")

enable_testing()

function(add_serial_order_test TEST_NAME)
  add_test(NAME ${TEST_NAME}
    COMMAND ${CMAKE_COMMAND}
      "-DTEST_OUTPUT_FILE=${TEST_OUTPUT_FILE}"
      "-DTEST_NAME=${TEST_NAME}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/test.cmake"
  )

  if(ARGC GREATER 1)
    set_tests_properties(${TEST_NAME} PROPERTIES ${ARGN})
  endif()
endfunction()

add_serial_order_test(initialization COST 1000)
add_serial_order_test(test1)
add_serial_order_test(test2)
add_serial_order_test(test3)
add_serial_order_test(test4 DEPENDS test5)

add_serial_order_test(test5)
set_tests_properties(test5 PROPERTIES DEPENDS "test6;test7b;test7a")

add_serial_order_test(test6 COST -2)
add_serial_order_test(test7a COST -1)
add_serial_order_test(test7b COST -1)
add_serial_order_test(test8 COST 10)
add_serial_order_test(test9 COST 20)
add_serial_order_test(test10 COST 0)
add_serial_order_test(test11)
add_serial_order_test(test12 COST 0)

add_serial_order_test(verification COST -1000)

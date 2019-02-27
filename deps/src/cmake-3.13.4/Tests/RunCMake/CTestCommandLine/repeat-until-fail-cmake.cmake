enable_testing()

set(TEST_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/test_output.txt")
add_test(NAME initialization
  COMMAND ${CMAKE_COMMAND}
  "-DTEST_OUTPUT_FILE=${TEST_OUTPUT_FILE}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/init.cmake")
add_test(NAME test1
  COMMAND ${CMAKE_COMMAND}
  "-DTEST_OUTPUT_FILE=${TEST_OUTPUT_FILE}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/test1.cmake")
set_tests_properties(test1 PROPERTIES DEPENDS "initialization")

add_test(hello ${CMAKE_COMMAND} -E echo hello)
set_tests_properties(hello PROPERTIES FAIL_REGULAR_EXPRESSION "hello.*hello")

add_test(goodbye ${CMAKE_COMMAND} -E echo goodbye)

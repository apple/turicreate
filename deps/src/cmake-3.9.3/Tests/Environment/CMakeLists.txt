cmake_minimum_required(VERSION 2.6)
project(EnvironmentProj)

add_executable(Environment main.cxx)

enable_testing()

add_test(Environment1 Environment)
add_test(Environment2 Environment)
add_test(EchoEnvironment1 ${CMAKE_COMMAND} -E environment)
add_test(EchoEnvironment2 ${CMAKE_COMMAND} -E environment)

# Make sure "CMAKE_ENV.*Happy Thanksgiving" is in the output of
# the "1" tests:
#
set_tests_properties(Environment1 EchoEnvironment1 PROPERTIES
  ENVIRONMENT "CMAKE_ENVIRONMENT_TEST_VAR=Happy Thanksgiving!"
  PASS_REGULAR_EXPRESSION "CMAKE_ENV.*Happy Thanksgiving"
)

# Make sure "CMAKE_ENV.*Happy Thanksgiving" is *NOT* in the output of
# the "2" tests:
#
set_tests_properties(Environment2 EchoEnvironment2 PROPERTIES
  FAIL_REGULAR_EXPRESSION "CMAKE_ENV.*Happy Thanksgiving"
)

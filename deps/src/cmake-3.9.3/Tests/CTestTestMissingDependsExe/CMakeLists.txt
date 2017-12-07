cmake_minimum_required(VERSION 2.8.12)

project(CTestTestMissingDependsExe)

enable_testing()

add_test(test1 ${CMAKE_COMMAND} -E echo test)
add_test(test2 non-existent-command)

set_tests_properties(test1 PROPERTIES DEPENDS test2)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/nodeprecated_defined)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/nodeprecated_not_defined)

configure_file(CMakeLists.txt.in ${CMAKE_CURRENT_BINARY_DIR}/nodeprecated_not_defined/CMakeLists.txt)
set(DEFINE_NO_DEPRECATED DEFINE_NO_DEPRECATED)
configure_file(CMakeLists.txt.in ${CMAKE_CURRENT_BINARY_DIR}/nodeprecated_defined/CMakeLists.txt)

try_compile(Result ${CMAKE_CURRENT_BINARY_DIR}/nodeprecated_not_defined_build
  ${CMAKE_CURRENT_BINARY_DIR}/nodeprecated_not_defined
  nodeprecated_test
  OUTPUT_VARIABLE Out
)

test_pass(Result "Failed to build without no-deprecated define")

try_compile(Result ${CMAKE_CURRENT_BINARY_DIR}/nodeprecated_defined_build
  ${CMAKE_CURRENT_BINARY_DIR}/nodeprecated_defined
  nodeprecated_test
  OUTPUT_VARIABLE Out
)

test_fail(Result "Built even with no-deprecated define")

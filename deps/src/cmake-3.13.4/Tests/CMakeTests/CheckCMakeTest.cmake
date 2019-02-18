get_filename_component(CMakeTests_SRC_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

function(check_cmake_test_single prefix test testfile)
  message(STATUS "Test ${prefix}-${test}...")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -P "${testfile}"
    WORKING_DIRECTORY "${CMakeTests_BIN_DIR}"
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    RESULT_VARIABLE result
    )
  string(REPLACE "\n" "\n out> " out " out> ${stdout}")
  string(REPLACE "\n" "\n err> " err " err> ${stderr}")
  if(NOT "${result}" STREQUAL "${${test}-RESULT}")
    message(FATAL_ERROR
      "Test ${test} result is [${result}], not [${${test}-RESULT}].\n"
      "Test ${test} output:\n"
      "${out}\n"
      "${err}")
  endif()
  if(${test}-STDERR AND NOT "${err}" MATCHES "${${test}-STDERR}")
    message(FATAL_ERROR
      "Test ${test} stderr does not match\n  ${${test}-STDERR}\n"
      "Test ${test} output:\n"
      "${out}\n"
      "${err}")
  endif()
endfunction()

function(check_cmake_test prefix)
  get_filename_component(CMakeTests_BIN_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
  foreach(test ${ARGN})
    check_cmake_test_single("${prefix}" "${test}" "${CMakeTests_SRC_DIR}/${prefix}-${test}.cmake")
  endforeach()
endfunction()

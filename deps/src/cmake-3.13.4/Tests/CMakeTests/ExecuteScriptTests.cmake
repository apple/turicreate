# This function calls the ${scriptname} file to execute one test case:
#
function(execute_one_script_test scriptname testname expected_result)
  message("execute_one_script_test")
  message("testname=[${testname}]")

  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -D "dir:STRING=${dir}"
      -D "testname:STRING=${testname}"
      -P "${scriptname}"
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    RESULT_VARIABLE result
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    )

  message("out=[${out}]")
  message("err=[${err}]")

  if(expected_result STREQUAL "fail")
    # case expected to fail, result should be non-0...
    # error if it's 0
    if("${result}" STREQUAL "0")
      message(SEND_ERROR "script failed: testname='${testname}' [${result}] actually passed, but expected to fail...")
    endif()
  else()
    # case expected to pass, result should be 0...
    # error if it's non-0
    if(NOT "${result}" STREQUAL "0")
      message(SEND_ERROR "script failed: testname='${testname}' [${result}] actually failed, but expected to pass...")
   endif()
  endif()

  message("")
endfunction()


# This function reads the script file and calls execute_one_script_test for
# each testname case listed in the script. To add new cases, simply edit the
# script file and add an elseif() clause that matches 'regex' below.
#
function(execute_all_script_tests scriptname result)
  file(READ "${scriptname}" script)

  string(REPLACE ";" "\\\\;" script "${script}")
  string(REPLACE "\n" "E;" script "${script}")

  set(count 0)
  set(regex "^ *(if|elseif) *\\( *testname +STREQUAL +\\\"*([^\\\"\\)]+)\\\"* *\\) *# *(fail|pass) *E$")

  foreach(line ${script})
    if(line MATCHES "${regex}")
      set(testname "${CMAKE_MATCH_2}")
      set(expected_result "${CMAKE_MATCH_3}")
      math(EXPR count "${count} + 1")
      execute_one_script_test(${scriptname} ${testname} ${expected_result})
    endif()
  endforeach()

  set(${result} ${count} PARENT_SCOPE)
endfunction()

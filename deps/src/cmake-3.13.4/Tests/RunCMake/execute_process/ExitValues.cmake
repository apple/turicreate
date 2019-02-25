#1st TEST RESULT_VARIABLE ONLY
execute_process(COMMAND ${EXIT_CODE_EXE} "zero_exit"
                RESULT_VARIABLE r0
                )
message(STATUS "  1 - 1 RESULT_VARIABLE: ${r0}")
if(NOT r0 EQUAL 0)
    message(FATAL_ERROR "zero exit code expected")
endif()
execute_process(COMMAND ${EXIT_CODE_EXE} "non_zero_exit"
                RESULT_VARIABLE r01
                ERROR_QUIET
                )
message(STATUS "  1 - 2 RESULT_VARIABLE: ${r01}")
if(r01 EQUAL 0)
    message(FATAL_ERROR "non-zero exit code expected")
endif()
#2nd TEST RESULT_VARIABLE and RESULTS_VARIABLE
execute_process(COMMAND ${EXIT_CODE_EXE} "zero_exit"
                RESULT_VARIABLE r1
                RESULTS_VARIABLE r1s
                )
message(STATUS "  2 - 1 RESULT_VARIABLE: ${r1}")
message(STATUS "  2 - 1 RESULTS_VARIABLE: ${r1s}")
if(NOT r1 EQUAL 0 OR NOT r1s EQUAL 0)
    message(FATAL_ERROR "zero exit code expected")
endif()
execute_process(COMMAND ${EXIT_CODE_EXE} "non_zero_exit"
                RESULT_VARIABLE r11
                RESULTS_VARIABLE r11s
                ERROR_QUIET
                )
message(STATUS "  2 - 2 RESULT_VARIABLE: ${r11}")
message(STATUS "  2 - 2 RESULTS_VARIABLE: ${r11s}")
if(r11 EQUAL 0 OR r11s EQUAL 0)
    message(FATAL_ERROR "non-zero exit code expected")
endif()
#3rd TEST RESULTS_VARIABLE
execute_process(COMMAND ${EXIT_CODE_EXE} "zero_exit"
                RESULTS_VARIABLE r2s
                )
message(STATUS "  3 - 1 RESULTS_VARIABLE: ${r2s}")
if(NOT r2s EQUAL 0)
    message(FATAL_ERROR "zero exit code expected")
endif()
execute_process(COMMAND ${EXIT_CODE_EXE} "non_zero_exit"
                RESULTS_VARIABLE r21s
                ERROR_QUIET
                )
message(STATUS "  3 - 2 RESULTS_VARIABLE: ${r21s}")
if(r21s EQUAL 0)
    message(FATAL_ERROR "non-zero exit code expected")
endif()
#4th TEST RESULT_VARIABLE and RESULTS_VARIABLE WITH MULTICOMMAND
execute_process(COMMAND ${EXIT_CODE_EXE} "non_zero_exit"
                COMMAND ${EXIT_CODE_EXE} "zero_exit"
                COMMAND ${EXIT_CODE_EXE} "non_zero_exit"
                COMMAND ${EXIT_CODE_EXE} "zero_exit"
                COMMAND ${EXIT_CODE_EXE} "non_zero_exit"
                COMMAND ${EXIT_CODE_EXE} "zero_exit"
                RESULT_VARIABLE  r31
                RESULTS_VARIABLE r31s
                OUTPUT_QUIET
                ERROR_QUIET
                )
message(STATUS "  4 - 1 RESULT_VARIABLE: ${r31}")
message(STATUS "  4 - 1 RESULTS_VARIABLE: ${r31s}")
if(NOT r31 EQUAL 0)
    message(FATAL_ERROR "zero exit code expected for last command")
endif()
list(LENGTH r31s r31sLen)
message(STATUS "  4 - 1 RESULTS_VARIABLE_LENGTH: ${r31sLen}")
if(NOT r31sLen EQUAL 6)
    message(FATAL_ERROR "length of RESULTS_VARIABLE is not as expected")
else()
    foreach(loop_var RANGE 5)
      list(GET r31s ${loop_var} rsLocal)
      math(EXPR isOdd "${loop_var} % 2")
      if(isOdd)
        if(NOT rsLocal EQUAL 0)
          message(FATAL_ERROR "zero exit code expected")
        endif()
      else()
        if(rsLocal EQUAL 0)
          message(FATAL_ERROR "non-zero exit code expected")
        endif()
      endif()
    endforeach()
endif()
#5th TEST RESULT_VARIABLE and RESULTS_VARIABLE WITH MULTICOMMAND
execute_process(COMMAND ${EXIT_CODE_EXE} "zero_exit"
                COMMAND ${EXIT_CODE_EXE} "zero_exit"
                COMMAND ${EXIT_CODE_EXE} "non_zero_exit"
                RESULT_VARIABLE  r41
                RESULTS_VARIABLE r41s
                OUTPUT_QUIET
                ERROR_QUIET
                )
message(STATUS "  5 - 1 RESULT_VARIABLE: ${r41}")
message(STATUS "  5 - 1 RESULTS_VARIABLE: ${r41s}")
if(r41 EQUAL 0)
    message(FATAL_ERROR "non-zero exit code expected for last command")
endif()
list(LENGTH r41s r41sLen)
message(STATUS "  5 - 1 RESULTS_VARIABLE_LENGTH: ${r41sLen}")
if(NOT r31sLen EQUAL 6)
    message(FATAL_ERROR "length of RESULTS_VARIABLE is not as expected")
else()
    list(GET r41s 0 rsLocal)
    if(NOT rsLocal EQUAL 0)
      message(FATAL_ERROR "zero exit code expected")
    endif()
    list(GET r41s 1 rsLocal)
    if(NOT rsLocal EQUAL 0)
      message(FATAL_ERROR "zero exit code expected")
    endif()
    list(GET r41s 2 rsLocal)
    if(rsLocal EQUAL 0)
      message(FATAL_ERROR "non-zero exit code expected")
    endif()
endif()

if(EXISTS ${RunCMake_TEST_BINARY_DIR}/check-debug.cmake)
  include(${RunCMake_TEST_BINARY_DIR}/check-debug.cmake)
  if(RunCMake_TEST_FAILED)
    return()
  endif()
  foreach(exe IN LISTS check_exes)
    execute_process(COMMAND ${exe} RESULT_VARIABLE res)
    if(NOT res EQUAL ${check_step})
      string(APPEND RunCMake_TEST_FAILED "
 '${exe}' returned '${res}' but expected '${check_step}'
")
    endif()
  endforeach()
  foreach(p IN LISTS check_pairs)
    if("${p}" MATCHES "^(.*)\\|(.*)$")
      set(lhs "${CMAKE_MATCH_1}")
      set(rhs "${CMAKE_MATCH_2}")
      if(NOT EXISTS "${lhs}")
        string(APPEND RunCMake_TEST_FAILED "
 '${lhs}' missing
")
      elseif(NOT EXISTS "${rhs}")
        string(APPEND RunCMake_TEST_FAILED "
 '${rhs}' missing
")
      elseif(NOT "${lhs}" IS_NEWER_THAN "${rhs}")
        string(APPEND RunCMake_TEST_FAILED "
 '${lhs}' is not newer than '${rhs}'
")
      endif()
    endif()
  endforeach()
else()
  set(RunCMake_TEST_FAILED "
 '${RunCMake_TEST_BINARY_DIR}/check-debug.cmake' missing
")
endif()

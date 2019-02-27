message(STATUS "testname='${testname}'")

if(testname STREQUAL bad_else) # fail
  file(WRITE "${dir}/${testname}.cmake"
"else()
")
  execute_process(COMMAND ${CMAKE_COMMAND} -P "${dir}/${testname}.cmake"
    RESULT_VARIABLE rv)
  if(NOT rv EQUAL 0)
    message(FATAL_ERROR "${testname} failed")
  endif()

elseif(testname STREQUAL bad_elseif) # fail
  file(WRITE "${dir}/${testname}.cmake"
"elseif()
")
  execute_process(COMMAND ${CMAKE_COMMAND} -P "${dir}/${testname}.cmake"
    RESULT_VARIABLE rv)
  if(NOT rv EQUAL 0)
    message(FATAL_ERROR "${testname} failed")
  endif()

elseif(testname STREQUAL bad_endforeach) # fail
  endforeach()

elseif(testname STREQUAL bad_endfunction) # fail
  endfunction()

elseif(testname STREQUAL bad_endif) # fail
  file(WRITE "${dir}/${testname}.cmake"
"cmake_minimum_required(VERSION 2.8)
endif()
")
  execute_process(COMMAND ${CMAKE_COMMAND} -P "${dir}/${testname}.cmake"
    RESULT_VARIABLE rv)
  if(NOT rv EQUAL 0)
    message(FATAL_ERROR "${testname} failed")
  endif()

elseif(testname STREQUAL endif_low_min_version) # pass
  file(WRITE "${dir}/${testname}.cmake"
"cmake_minimum_required(VERSION 1.2)
endif()
")
  execute_process(COMMAND ${CMAKE_COMMAND} -P "${dir}/${testname}.cmake"
    RESULT_VARIABLE rv)
  if(NOT rv EQUAL 0)
    message(FATAL_ERROR "${testname} failed")
  endif()

elseif(testname STREQUAL endif_no_min_version) # pass
  file(WRITE "${dir}/${testname}.cmake"
"endif()
")
  execute_process(COMMAND ${CMAKE_COMMAND} -P "${dir}/${testname}.cmake"
    RESULT_VARIABLE rv)
  if(NOT rv EQUAL 0)
    message(FATAL_ERROR "${testname} failed")
  endif()

elseif(testname STREQUAL bad_endmacro) # fail
  endmacro()

elseif(testname STREQUAL bad_endwhile) # fail
  endwhile()

else() # fail
  message(FATAL_ERROR "testname='${testname}' - error: no such test in '${CMAKE_CURRENT_LIST_FILE}'")

endif()

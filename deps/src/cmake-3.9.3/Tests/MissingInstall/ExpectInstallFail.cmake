if(MULTI_CONFIG)
  set(SI_CONFIG --config $<CONFIGURATION>)
else()
  set(SI_CONFIG)
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND}
    --build .
    --target install ${SI_CONFIG}
  RESULT_VARIABLE RESULT
  OUTPUT_VARIABLE OUTPUT
  ERROR_VARIABLE ERROR
)

if(RESULT EQUAL 0)
  message(FATAL_ERROR "install should have failed")
endif()

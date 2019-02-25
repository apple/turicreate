execute_process(COMMAND ${MYEXECUTABLE} RESULT_VARIABLE RESULT)

if(NOT RESULT_VARIABLE STREQUAL "0")
  message("Test failure")
endif()

file(GLOB_RECURSE gcno_files "${TARGETDIR}/*.gcno")

foreach(gcno_file ${gcno_files})
  string(REPLACE ".gcno" ".gcda" gcda_file "${gcno_file}")
  configure_file(${gcno_file} ${gcda_file} COPYONLY)
endforeach()

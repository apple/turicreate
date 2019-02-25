message("This script could run an external packaging tool")

function(expect_variable VAR)
  if(NOT ${VAR})
    message(FATAL_ERROR "${VAR} is unexpectedly not set")
  endif()
endfunction()

function(expect_file FILE)
  if(NOT EXISTS "${FILE}")
    message(FATAL_ERROR "${FILE} is unexpectedly missing")
  endif()
endfunction()

expect_variable(CPACK_COMPONENTS_ALL)
expect_variable(CPACK_TOPLEVEL_DIRECTORY)
expect_variable(CPACK_TEMPORARY_DIRECTORY)
expect_variable(CPACK_PACKAGE_DIRECTORY)
expect_variable(CPACK_PACKAGE_FILE_NAME)

expect_file(${CPACK_TEMPORARY_DIRECTORY}/f1/share/cpack-test/f1.txt)
expect_file(${CPACK_TEMPORARY_DIRECTORY}/f2/share/cpack-test/f2.txt)
expect_file(${CPACK_TEMPORARY_DIRECTORY}/f3/share/cpack-test/f3.txt)
expect_file(${CPACK_TEMPORARY_DIRECTORY}/f4/share/cpack-test/f4.txt)

message(STATUS "This status message is expected to be visible")

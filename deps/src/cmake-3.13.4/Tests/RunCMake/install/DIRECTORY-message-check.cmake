file(REMOVE_RECURSE ${RunCMake_TEST_BINARY_DIR}/prefix)
execute_process(COMMAND ${CMAKE_COMMAND} -P ${RunCMake_TEST_BINARY_DIR}/cmake_install.cmake
  OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(expect "
-- Installing: [^\n]*/prefix/dir\r?
-- Installing: [^\n]*/prefix/dir/empty.txt\r?
")
if(NOT out MATCHES "${expect}")
  string(REGEX REPLACE "\n" "\n  " out "  ${out}")
  string(APPEND RunCMake_TEST_FAILED
    "First install did not say 'Installing' as expected:\n${out}")
endif()
set(f ${RunCMake_TEST_BINARY_DIR}/prefix/dir/empty.txt)
if(NOT EXISTS "${f}")
  string(APPEND RunCMake_TEST_FAILED
    "File was not installed:\n  ${f}\n")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -P ${RunCMake_TEST_BINARY_DIR}/cmake_install.cmake
  OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(expect "
-- Up-to-date: [^\n]*/prefix/dir\r?
-- Up-to-date: [^\n]*/prefix/dir/empty.txt\r?
")
if(NOT out MATCHES "${expect}")
  string(REGEX REPLACE "\n" "\n  " out "  ${out}")
  string(APPEND RunCMake_TEST_FAILED
    "Second install did not say 'Up-to-date' as expected:\n${out}")
endif()

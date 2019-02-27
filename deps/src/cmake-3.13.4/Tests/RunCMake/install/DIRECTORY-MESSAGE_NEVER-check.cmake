file(REMOVE_RECURSE ${RunCMake_TEST_BINARY_DIR}/prefix)
execute_process(COMMAND ${CMAKE_COMMAND} -P ${RunCMake_TEST_BINARY_DIR}/cmake_install.cmake
  OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(out MATCHES "-- Installing: [^\n]*prefix/dir")
  string(REGEX REPLACE "\n" "\n  " out "  ${out}")
  string(APPEND RunCMake_TEST_FAILED
    "Installation output was not quiet:\n${out}")
endif()
set(f ${RunCMake_TEST_BINARY_DIR}/prefix/dir/empty.txt)
if(NOT EXISTS "${f}")
  string(APPEND RunCMake_TEST_FAILED
    "File was not installed:\n  ${f}\n")
endif()

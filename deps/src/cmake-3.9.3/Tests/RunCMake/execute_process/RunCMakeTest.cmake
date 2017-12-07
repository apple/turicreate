include(RunCMake)

set(RunCMake_TEST_OUTPUT_MERGE 1)
run_cmake_command(MergeOutput ${CMAKE_COMMAND} -P ${RunCMake_SOURCE_DIR}/MergeOutput.cmake)
unset(RunCMake_TEST_OUTPUT_MERGE)

run_cmake_command(MergeOutputFile ${CMAKE_COMMAND} -P ${RunCMake_SOURCE_DIR}/MergeOutputFile.cmake)
run_cmake_command(MergeOutputVars ${CMAKE_COMMAND} -P ${RunCMake_SOURCE_DIR}/MergeOutputVars.cmake)

run_cmake(EncodingMissing)
if(TEST_ENCODING_EXE)
  run_cmake_command(EncodingUTF8 ${CMAKE_COMMAND} -DTEST_ENCODING=UTF8 -DTEST_ENCODING_EXE=${TEST_ENCODING_EXE} -P ${RunCMake_SOURCE_DIR}/Encoding.cmake)
endif()
